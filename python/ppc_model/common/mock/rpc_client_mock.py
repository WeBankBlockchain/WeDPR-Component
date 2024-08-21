import threading

from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest, ModelResponse
from ppc_model.interface.rpc_client import RpcClient


class RpcClientMock(RpcClient):
    def __init__(self, need_failed=False):
        self._need_failed = need_failed
        self._bad_guy = 0
        self._lock = threading.Lock()
        self._on_message_received = None

    def set_message_handler(self, on_message_received):
        self._on_message_received = on_message_received

    def send(self, request: ModelRequest):
        # print(
        #     f"send data to {request.receiver}, task_id: {request.task_id}, "
        #     f"key: {request.key}, seq: {request.seq}")
        self._on_message_received(request)
        response = ModelResponse()
        if self._need_failed:
            # 模拟网络断连
            with self._lock:
                self._bad_guy += 1
                response.base_response.error_code = self._bad_guy % 2
        else:
            response.base_response.error_code = 0
            response.base_response.message = "success"
        return response
