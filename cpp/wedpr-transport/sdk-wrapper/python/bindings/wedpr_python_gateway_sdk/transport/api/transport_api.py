# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI


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
