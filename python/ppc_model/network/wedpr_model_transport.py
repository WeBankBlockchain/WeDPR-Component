# -*- coding: utf-8 -*-
from wedpr_python_gateway_sdk.transport.impl.transport import Transport
from wedpr_python_gateway_sdk.transport.impl.transport import RouteType
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from ppc_model.network.wedpr_model_transport_api import ModelTransportApi
from ppc_model.network.wedpr_model_transport_api import ModelRouterApi
from ppc_model.task.task_manager import TaskManager
import time
from readerwriterlock import rwlock
from enum import Enum


class ModelTransport(ModelTransportApi):
    def __init__(self, transport: Transport, self_agency_id: str, task_manager: TaskManager,
                 component_type,
                 send_msg_timeout_ms: int = 5000,
                 pop_msg_timeout_ms: int = 60000):
        self.transport = transport
        self.self_agency_id = self_agency_id
        # default send msg timeout
        self.send_msg_timeout = send_msg_timeout_ms
        self.pop_msg_timeout = pop_msg_timeout_ms
        self.task_manager = task_manager
        self.component_type = component_type

    @staticmethod
    def get_topic(task_id: str, task_type: str, agency: str):
        return f"{agency}_{task_id}_{task_type}"

    @staticmethod
    def get_topic_without_agency(task_id: str, task_type: str):
        return f"{task_id}_{task_type}"

    def push_by_component(self, task_id: str, task_type: str, dst_inst: str, data):
        self.transport.push_by_component(topic=self.get_topic(task_id, task_type, dst_inst),
                                         dstInst=dst_inst,
                                         component=self.component_type,
                                         payload=data, timeout=self.send_msg_timeout)

    def push_by_nodeid(self, task_id: str, task_type: str, dst_node: str, payload: bytes, seq: int = 0):
        self.transport.push_by_nodeid(topic=self.get_topic(task_id, task_type, self.self_agency_id),
                                      dstNode=bytes(
                                          dst_node, encoding="utf-8"),
                                      seq=seq, payload=payload,
                                      timeout=self.send_msg_timeout)

    def pop_by_topic(self, topic, task_id) -> MessageAPI:
        while not self.task_manager.task_finished(task_id):
            msg = self.transport.pop(
                topic=topic, timeout_ms=self.pop_msg_timeout)
            # wait for the msg if the task is running
            if msg is None:
                time.sleep(0.04)
            else:
                return msg
        raise Exception(f"Not receive the message of topic:"
                        f" {topic} "
                        f"even after the task has been killed!")

    def pop(self, task_id: str, task_type: str, dst_inst: str) -> MessageAPI:
        return self.pop_by_topic(topic=self.get_topic(task_id, task_type, dst_inst), task_id=task_id)

    def get_component_type(self):
        return self.component_type

    def select_node(self, route_type: RouteType, dst_agency: str, dst_component: str) -> str:
        return self.transport.select_node_by_route_policy(route_type=route_type,
                                                          dst_inst=dst_agency, dst_component=dst_component)

    def stop(self):
        self.transport.stop()


class BaseMessage(Enum):
    Handshake = "Handshake"


class ModelRouter(ModelRouterApi):
    def __init__(self, logger, transport: ModelTransport):
        self.logger = logger
        self.transport = transport
        # task_id=>{agency=>selectedNode}
        self.router_info = {}
        self._rw_lock = rwlock.RWLockWrite()

    def handshake(self, task_id, participant):
        self.logger.info(f"handshake with {participant}")
        topic = ModelTransport.get_topic_without_agency(
            task_id, BaseMessage.Handshake.value)
        self.transport.transport.register_topic(topic)
        self.transport.transport.push_by_topic(topic=topic,
                                               dstInst=participant,
                                               seq=0, payload=bytes(),
                                               timeout=self.transport.send_msg_timeout)

    def __all_connected__(self, task_id, participant_id_list, self_agency_id):
        with self._rw_lock.gen_rlock():
            if task_id not in self.router_info.keys():
                return False
            for participant in participant_id_list:
                if participant == self_agency_id:
                    continue
                if participant not in self.router_info.get(task_id).keys():
                    return False
            self.logger.info(
                f"__all_connected__, task: {task_id}, participant_id_list: {participant_id_list}")
            return True

    def wait_for_handshake(self, task_id, participant_id_list: list, self_agency_id):
        while not self.__all_connected__(task_id, participant_id_list, self_agency_id):
            time.sleep(0.04)
            topic = ModelTransport.get_topic_without_agency(
                task_id, BaseMessage.Handshake.value)
            self.transport.transport.register_topic(topic)
            result = self.transport.pop_by_topic(topic=topic, task_id=task_id)

            if result is None:
                raise Exception(f"wait_for_handshake failed!")
            self.logger.info(
                f"wait_for_handshake success, task: {task_id}, detail: {result}")
            with self._rw_lock.gen_wlock():
                from_inst = result.get_header().get_src_inst()
                if task_id not in self.router_info.keys():
                    self.router_info.update({task_id: dict()})
                self.router_info.get(task_id).update(
                    {from_inst: result.get_header().get_src_node().decode("utf-8")})

    def on_task_finish(self, task_id):
        topic = ModelTransport.get_topic_without_agency(
            task_id, BaseMessage.Handshake.value)
        self.transport.transport.unregister_topic(topic)
        with self._rw_lock.gen_wlock():
            if task_id in self.router_info.keys():
                self.router_info.pop(task_id)

    def __get_dstnode__(self, task_id, dst_agency):
        with self._rw_lock.gen_rlock():
            if task_id in self.router_info.keys() and dst_agency in self.router_info.get(task_id).keys():
                return self.router_info.get(task_id).get(dst_agency)
        raise Exception(
            f"No Router  found for task {task_id}, dst_agency: {dst_agency}")

    def push(self, task_id: str, task_type: str, dst_agency: str, payload: bytes, seq: int = 0):
        dst_node = self.__get_dstnode__(task_id, dst_agency)
        self.transport.push_by_nodeid(
            task_id=task_id, task_type=task_type, dst_node=dst_node, payload=payload, seq=seq)

    def pop(self, task_id: str, task_type: str, from_inst: str) -> bytes:
        result = self.transport.pop(
            task_id=task_id, task_type=task_type, dst_inst=from_inst)
        self.logger.debug(f"Receive message from {result}")
        return result.get_payload()
