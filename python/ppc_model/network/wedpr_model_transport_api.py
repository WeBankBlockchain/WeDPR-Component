# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod


class ModelTransportApi(ABC):
    @abstractmethod
    def push_by_nodeid(self, task_id: str, task_type: str, dst_node: str, payload: bytes, seq: int = 0):
        pass

    @abstractmethod
    def pop(self, task_id: str, task_type: str, dst_inst: str):
        pass


class ModelRouterApi:
    @abstractmethod
    def push(self, task_id: str, task_type: str, dst_agency: str, payload: bytes, seq: int = 0):
        pass

    @abstractmethod
    def pop(self, task_id: str, task_type: str, from_inst: str) -> bytes:
        pass
