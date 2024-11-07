# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod
import json
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.utils.base_object import BaseObject


class EntryPointInfo(BaseObject):
    def __init__(self, service_name: str = None, entrypoint: str = None):
        self.serviceName = service_name
        self.entryPoint = entrypoint

    def __repr__(self):
        return f"serviceName: {self.serviceName}, entryPoint: {self.entryPoint}"


class ServiceMeta(BaseObject):
    SERVICE_INFOS_KEY = "serviceInfos"

    def __init__(self, entrypoints_info: [EntryPointInfo]):
        self.serviceInfos = entrypoints_info

    def encode(self) -> str:
        service_meta_dict = {}
        if self.serviceInfos is None:
            return ""
        service_info_list = []
        for service_info in self.serviceInfos:
            service_info_list.append(service_info.as_dict())
        service_meta_dict.update(
            {ServiceMeta.SERVICE_INFOS_KEY: service_info_list})
        return json.dumps(service_meta_dict)

    @staticmethod
    def decode(json_str):
        if json_str is None or len(json_str) == 0:
            return None
        service_meta = ServiceMeta([])
        service_meta_dict = json.loads(json_str)
        if ServiceMeta.SERVICE_INFOS_KEY in service_meta_dict.keys():
            service_info_list = service_meta_dict.get(
                ServiceMeta.SERVICE_INFOS_KEY)
            for service_info in service_info_list:
                entrypoint_info = EntryPointInfo()
                entrypoint_info.set_params(**service_info)
                service_meta.serviceInfos.append(entrypoint_info)
        return service_meta

    def __repr__(self):
        repr_str = "{"
        if self.serviceInfos is None:
            return repr_str
        for service_info in self.serviceInfos:
            repr_str = f"{repr_str}, [{service_info}]"
        repr_str = f"{repr_str}" + "}"
        return repr_str


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
    def get_alive_entrypoints(self, service_name: str) -> [EntryPointInfo]:
        pass

    @abstractmethod
    def register_service_info(self, service, entrypoint):
        pass
