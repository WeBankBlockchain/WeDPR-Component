# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod


class MessageHeaderAPI(ABC):
    @abstractmethod
    def get_version(self) -> str:
        pass

    @abstractmethod
    def get_trace_id(self) -> str:
        pass

    @abstractmethod
    def get_src_gw_node(self) -> str:
        pass

    @abstractmethod
    def get_dst_gw_node(self) -> str:
        pass

    @abstractmethod
    def get_packet_type(self) -> int:
        pass

    @abstractmethod
    def get_ttl(self) -> int:
        pass

    @abstractmethod
    def get_ext(self) -> int:
        pass

    @abstractmethod
    def is_resp_packet(self) -> bool:
        pass

    @abstractmethod
    def get_route_type(self) -> int:
        pass

    @abstractmethod
    def get_component_type(self) -> str:
        pass

    @abstractmethod
    def get_src_node(self) -> bytes:
        pass

    @abstractmethod
    def get_dst_node(self) -> bytes:
        pass

    @abstractmethod
    def get_dst_inst(self) -> str:
        pass

    @abstractmethod
    def get_src_inst(self) -> str:
        pass

    @abstractmethod
    def get_topic(self) -> str:
        pass


class MessageAPI(ABC):

    @abstractmethod
    def get_header(self) -> MessageHeaderAPI:
        pass

    @abstractmethod
    def get_seq(self) -> int:
        pass

    @abstractmethod
    def get_payload(self) -> bytes:
        pass

    @abstractmethod
    def get_length(self) -> int:
        pass
