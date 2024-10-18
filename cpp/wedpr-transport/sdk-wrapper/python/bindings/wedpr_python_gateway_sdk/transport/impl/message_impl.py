# -*- coding: utf-8 -*-

from wedpr_python_gateway_sdk.transport.api.message_api import MessageHeaderAPI
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Message
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import MessageHeader


class MessageHeaderImpl(MessageHeaderAPI):

    def __init__(self, header: MessageHeader):
        if header is None:
            raise Exception(
                "Build MessageHeader failed for empty header object passed in")
        self.__header = header

    def get_version(self) -> str:
        return self.__header.version()

    def get_trace_id(self) -> str:
        return self.__header.traceID()

    def get_src_gw_node(self) -> str:
        return self.__header.srcGwNode()

    def get_dst_gw_node(self) -> str:
        return self.__header.dstGwNode()

    def get_packet_type(self) -> int:
        return self.__header.packetType()

    def get_ttl(self) -> int:
        return self.__header.ttl()

    def get_ext(self) -> int:
        return self.__header.exit()

    def is_resp_packet(self) -> bool:
        return self.__header.isRespPacket()

    def get_route_type(self) -> int:
        return self.__header.routeType()

    def get_component_type(self) -> str:
        return self.__header.optionalField().componentType()

    def get_src_node(self) -> bytes:
        return self.__header.optionalField().srcNodeBuffer()

    def get_dst_node(self) -> bytes:
        return self.__header.optionalField().dstNodeBuffer()

    def get_dst_inst(self) -> str:
        return self.__header.optionalField().dstInst()

    def get_src_inst(self) -> str:
        return self.__header.optionalField().srcInst()

    def get_topic(self) -> str:
        return self.__header.optionalField().topic()

    def detail(self) -> str:
        return f"version: {self.get_version()}, topic: {self.get_topic()}, src_inst: {self.get_src_inst()},src_node: {str(self.get_src_node())}, dst_inst: {self.get_dst_inst()}, dst_node: {str(self.get_dst_node())}"


class MessageImpl(MessageAPI):
    def __init__(self, message: Message):
        if message is None:
            raise Exception(
                "Build Message failed for empty message object passed in")
        self.__message = message
        self.__message_header = MessageHeaderImpl(self.__message.header())

    def get_header(self) -> MessageHeaderAPI:
        return self.__message_header

    def get_seq(self) -> int:
        return self.__message.frontMessage().seq()

    def get_payload(self) -> bytes:
        return self.__message.frontMessage().dataBuffer()

    def get_length(self) -> int:
        return self.__message.frontMessage().length()

    def detail(self) -> str:
        return f"seq: {self.get_seq()}, header: {self.get_header().detail()}, length: {self.get_length()}"
