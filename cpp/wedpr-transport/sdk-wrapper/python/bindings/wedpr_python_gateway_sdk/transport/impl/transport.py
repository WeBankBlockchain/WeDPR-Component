# -*- coding: utf-8 -*-

from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Transport
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Error
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.transport.impl.route_info_builder import RouteInfoBuilder
from wedpr_python_gateway_sdk.transport.impl.message_factory import MessageFactory
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import MessageOptionalHeader
from wedpr_python_gateway_sdk.transport.api.transport_api import TransportAPI

from enum import Enum
import signal


class RouteType(Enum):
    ROUTE_THROUGH_NODEID = 0
    ROUTE_THROUGH_COMPONENT = 1
    ROUTE_THROUGH_AGENCY = 2
    ROUTE_THROUGH_TOPIC = 3


class Transport(TransportAPI):
    should_exit = False

    def __init__(self, transport: Transport):
        self.__transport = transport
        self.__route_info_builder = RouteInfoBuilder(
            self.__transport.routeInfoBuilder())

    def start(self):
        self.__transport.start()

    def stop(self):
        self.__transport.stop()

    def _push_msg(self, route_type: int, route_info: MessageOptionalHeader, payload: bytes, seq: int, timeout: int):
        try:
            return self.__transport.getFront().push_msg(route_type, route_info, payload, seq, timeout)
        except Exception as e:
            raise e

    def push_by_nodeid(self, topic: str, dstNode: bytes, seq: int, payload: bytes, timeout: int):
        route_info = self.__route_info_builder.build(
            topic=topic, dst_node=dstNode, dst_inst=None, component=None)
        result = self._push_msg(
            RouteType.ROUTE_THROUGH_NODEID.value, route_info, payload, seq, timeout)
        Transport.check_result("push_by_nodeid", result)

    def push_by_inst(self, topic: str, dstInst: str, seq: int, payload: bytes, timeout: int):
        route_info = self.__route_info_builder.build(
            topic=topic, dst_node=None, dst_inst=dstInst, component=None)
        result = self._push_msg(
            RouteType.ROUTE_THROUGH_AGENCY.value, route_info, payload, len(payload), seq, timeout)
        Transport.check_result("push_by_inst", result)

    def push_by_component(self, topic: str, dstInst: str,  component: str, seq: int, payload: bytes, timeout: int):
        route_info = self.__route_info_builder.build(
            topic=topic, dst_node=None, dst_inst=dstInst, component=component)
        result = self._push_msg(
            RouteType.ROUTE_THROUGH_COMPONENT.value, route_info, payload, len(payload), seq, timeout)
        Transport.check_result("push_by_component", result)

    def pop(self, topic, timeout_ms) -> MessageAPI:
        return MessageFactory.build(self.__transport.getFront().pop(topic, timeout_ms))

    def peek(self, topic):
        return self.__transport.peek(topic)

    @staticmethod
    def check_result(method: str, result: Error):
        if result is None:
            return
        if result.errorCode() != 0:
            raise Exception(
                f"call {method} failed for {result.errorMessage()}, code: {result.errorCode()}")

    def register_topic(self, topic):
        result = self.__transport.getFront().registerTopic(topic)
        Transport.check_result("register_topic", result)

    def unregister_topic(self, topic):
        result = self.__transport.getFront().unRegisterTopic(topic)
        Transport.check_result("unregister_topic", result)

    def register_component(self, component):
        result = self.__transport.getFront().registerComponent(component)
        Transport.check_result("register_component", result)

    def unregister_component(self, component):
        result = self.__transport.getFront().unRegisterComponent(component)
        Transport.check_result("unregister_component", result)


def signal_handler(sig, frame):
    print('You pressed Ctrl+C! Exiting gracefully.')
    Transport.should_exit = True


signal.signal(signal.SIGINT, signal_handler)
