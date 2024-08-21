import os
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from typing import Dict, Union

from readerwriterlock import rwlock

from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode


@dataclass
class PushRequest:
    receiver: str  # 数据接收方的机构ID
    task_id: str
    key: str  # 数据键
    data: bytes  # 二进制数据
    slice_size_MB: int = 2  # 切片大小，默认为2MB

    def slice_data(self):
        """将 data 按 slice_size 进行切片"""
        if not self.data:
            return [b'']
        slice_size = self.slice_size_MB * 1024 * 1024
        return [self.data[i:i + slice_size] for i in
                range(0, len(self.data), slice_size)]


@dataclass
class PullRequest:
    sender: str  # 数据发送方的机构ID
    task_id: str
    key: str  # 数据键


class ModelStub:
    def __init__(
            self,
            agency_id: str,
            thread_event_manager: ThreadEventManager,
            rpc_client,
            send_retry_times: int = 3,
            retry_interval_s: Union[int, float] = 5
    ) -> None:
        self.agency_id = agency_id
        self._thread_event_manager = thread_event_manager
        self._rpc_client = rpc_client
        self._executor = ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1))
        self._send_retry_times = send_retry_times
        self._retry_interval_s = retry_interval_s
        # 缓存收到的消息 [task_id:[sender:[key:[seq: data]]]]
        # 缓存清理由TaskManager完成
        self._received_data: Dict[str, Dict[str,
                                            Dict[str, Dict[int, tuple[int, bytes]]]]] = {}
        self._received_uuid: Dict[str, set[str]] = {}
        self._traffic_volume: Dict[str, int] = {}
        self._data_rw_lock = rwlock.RWLockWrite()

    def push(self, request: PushRequest) -> bytes:
        """
        发送消息
        param request: 消息请求
        """
        slices = request.slice_data()
        futures = []
        for seq, data in enumerate(slices):
            model_request = ModelRequest(
                sender=self.agency_id,
                receiver=request.receiver,
                task_id=request.task_id,
                key=request.key,
                seq=seq,
                slice_num=len(slices),
                data=data
            )
            future = self._executor.submit(
                self._send_with_retry, model_request)
            futures.append(future)

        ret = bytearray()
        for future in futures:
            ret.extend(future.result())

        self._accumulate_traffic_volume(request.task_id, len(request.data))

        return bytes(ret)

    def pull(self, pull_request: PullRequest) -> bytes:
        """
        接收消息
        param request: 待收消息元信息
        return 消息
        """
        task_id = pull_request.task_id
        sender = pull_request.sender
        key = pull_request.key
        while not self._thread_event_manager.event_status(task_id):
            if self._is_all_data_ready(pull_request):
                ret = bytearray()
                with self._data_rw_lock.gen_rlock():
                    slice_num = len(self._received_data[task_id][sender][key])
                    for seq in range(slice_num):
                        ret.extend(
                            self._received_data[task_id][sender][key][seq][1])
                # 缓存中删除已获取到的数据
                with self._data_rw_lock.gen_wlock():
                    del self._received_data[task_id][sender][key]
                self._accumulate_traffic_volume(task_id, len(ret))
                return bytes(ret)
            # 任务还在执行, 休眠后继续尝试获取数据
            time.sleep(0.04)

        # 接收到杀任务的信号
        raise PpcException(PpcErrorCode.TASK_IS_KILLED.get_code(),
                           PpcErrorCode.TASK_IS_KILLED.get_msg())

    def traffic_volume(self, task_id) -> float:
        with self._data_rw_lock.gen_rlock():
            if task_id not in self._traffic_volume:
                return 0
            return self._traffic_volume[task_id] / 1024 / 1024

    def on_message_received(self, model_request: ModelRequest):
        """
        注册给服务端的回调，服务端收到消息后调用
        param model_request: 收到的消息
        """
        # 消息幂等
        if not self._is_new_data(model_request):
            return
        # 缓存数据
        self._handle_received_data(model_request)

    def cleanup_cache(self, task_id):
        with self._data_rw_lock.gen_wlock():
            if task_id in self._received_data:
                del self._received_data[task_id]
            if task_id in self._received_uuid:
                del self._received_uuid[task_id]
            if task_id in self._traffic_volume:
                del self._traffic_volume[task_id]

    def _is_new_data(self, model_request: ModelRequest) -> bool:
        # 返回是否需要继续处理消息
        task_id = model_request.task_id
        uuid = f"{task_id}:{model_request.sender}:{model_request.key}:{model_request.seq}"
        with self._data_rw_lock.gen_wlock():
            if task_id in self._received_uuid and uuid in self._received_uuid[task_id]:
                # 收到重复的消息
                return False
            elif task_id in self._received_uuid and uuid not in self._received_uuid[task_id]:
                # 收到task_id的新消息
                self._received_uuid[task_id].add(uuid)
            else:
                # 首次收到task_id的消息
                self._received_uuid[task_id] = {uuid}
            return True

    def _handle_received_data(self, model_request: ModelRequest):
        task_id = model_request.task_id
        sender = model_request.sender
        key = model_request.key
        seq = model_request.seq
        slice_num = model_request.slice_num
        data = model_request.data
        with self._data_rw_lock.gen_wlock():
            if task_id not in self._received_data:
                self._received_data[task_id] = {
                    model_request.sender: {key: {seq: (slice_num, data)}}}
            elif sender not in self._received_data[task_id]:
                self._received_data[task_id][sender] = {
                    key: {seq: (slice_num, data)}}
            elif key not in self._received_data[task_id][sender]:
                self._received_data[task_id][sender][key] = {
                    seq: (slice_num, data)}
            else:
                self._received_data[task_id][sender][key][seq] = (
                    slice_num, data)

    def _is_all_data_ready(self, pull_request: PullRequest):
        task_id = pull_request.task_id
        sender = pull_request.sender
        key = pull_request.key
        with self._data_rw_lock.gen_rlock():
            if task_id not in self._received_data:
                return False
            if sender not in self._received_data[task_id]:
                return False
            if key not in self._received_data[task_id][sender]:
                return False
            if len(self._received_data[task_id][sender][key]) == 0:
                return False
            _, first_value = next(
                iter(self._received_data[task_id][sender][key].items()))
            if first_value[0] != len(self._received_data[task_id][sender][key]):
                return False
            return True

    def _send_with_retry(self, model_request: ModelRequest):
        retry_times = 0
        while retry_times <= self._send_retry_times:
            model_response = self._rpc_client.send(model_request)
            if model_response.base_response.error_code == 0:
                return model_response.data
            if retry_times <= self._send_retry_times:
                retry_times += 1
                time.sleep(self._retry_interval_s)
            else:
                raise PpcException(PpcErrorCode.NETWORK_ERROR.get_code(
                ), model_response.base_response.message)

    def _accumulate_traffic_volume(self, task_id, length):
        with self._data_rw_lock.gen_wlock():
            if task_id not in self._traffic_volume:
                self._traffic_volume[task_id] = 0
            self._traffic_volume[task_id] += length
