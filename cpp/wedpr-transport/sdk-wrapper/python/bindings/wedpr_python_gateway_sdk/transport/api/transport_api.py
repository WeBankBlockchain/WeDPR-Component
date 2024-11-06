# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod
import json
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.utils.base_object import BaseObject


class EntryPointInfo(BaseObject):
    def __init__(self, service_name: str, entrypoint: str):
        self.service_name = service_name
        self.entrypoint = entrypoint


class ServiceMeta(BaseObject):
    def __init__(self, entrypoints_info: list[EntryPointInfo]):
        self.entrypoints_info = entrypoints_info


def str_to_service_meta(json_str: str) -> ServiceMeta:
    if json_str is None or len(json_str) == 0:
        return None
    meta_dict = json.loads(json_str)
    service_meta = ServiceMeta([])
    service_meta.set_params(**meta_dict)
    return service_meta


class TransportAPI(ABC):
    @abstractmethod
    def start(self):
        pass

    @abstractmethod
    def stop(self):
        pass

    @abstractmethod
    def push_by_nodeid(self, topic: str, dstNode: bytes, seq: int, payload: bytes, timeout: int):
        pass

    @abstractmethod
    def push_by_inst(self, topic: str, dstInst: str, seq: int, payload: bytes, timeout: int):
        pass

    @abstractmethod
    def push_by_component(self, topic: str, dstInst: str,  component: str, seq: int, payload: bytes, timeout: int):
        pass

    @abstractmethod
    def push_by_topic(self, topic: str, dstInst: str, seq: int, payload: bytes, timeout: int):
        pass

    @abstractmethod
    def pop(self, topic, timeoutMs) -> MessageAPI:
        pass

    @abstractmethod
    def peek(self, topic) -> MessageAPI:
        pass

    @abstractmethod
    def register_topic(self, topic):
        pass

    @abstractmethod
    def unregister_topic(self, topic):
        pass

    @abstractmethod
    def register_component(self, component):
        pass

    @abstractmethod
    def unregister_component(self, component):
        pass

    @abstractmethod
    def get_alive_entrypoints(self, service_name: str) -> list[EntryPointInfo]:
        pass
