# -*- coding: utf-8 -*-

from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Transport
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import StringVec
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Error
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.transport.impl.route_info_builder import RouteInfoBuilder
from wedpr_python_gateway_sdk.transport.impl.message_factory import MessageFactory
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import MessageOptionalHeader
from wedpr_python_gateway_sdk.transport.api.transport_api import TransportAPI
from wedpr_python_gateway_sdk.transport.impl.transport_config import TransportConfig
import random
from wedpr_python_gateway_sdk.transport.api.transport_api import EntryPointInfo
from wedpr_python_gateway_sdk.transport.api.transport_api import ServiceMeta
from enum import Enum
import signal


class RouteType(Enum):
    ROUTE_THROUGH_NODEID = 0
    ROUTE_THROUGH_COMPONENT = 1
    ROUTE_THROUGH_AGENCY = 2
    ROUTE_THROUGH_TOPIC = 3


class Transport(TransportAPI):
    should_exit = False

    def __init__(self, transport: Transport, transport_config: TransportConfig):
        self.__transport = transport
        self.__transport_config = transport_config
        self.__route_info_builder = RouteInfoBuilder(
            self.__transport.routeInfoBuilder())
        self.service_meta = transport_config.service_meta

    def get_config(self) -> TransportConfig:
        return self.__transport_config

    def start(self):
        self.__transport.start()

    def stop(self):
        self.__transport.stop()

    def _push_msg(self, route_type: int, route_info: MessageOptionalHeader, payload: bytes, seq: int, timeout: int):
        try:
            return self.__transport.getFront().push_msg(route_type, route_info, payload, seq, timeout)
        except Exception as e:
            raise e

    def select_node_list_by_route_policy(self, route_type: RouteType, dst_inst: str, dst_component: str, dst_node: str = None) -> tuple:
        dst_node_bytes = None
        if dst_node is not None:
            dst_node_bytes = bytes(dst_node, encodings="utf-8")
        route_info = self.__route_info_builder.build(topic=None, dst_inst=dst_inst,
                                                     component=dst_component, dst_node=dst_node_bytes)
        try:
            return self.__transport.getFront().selectNodesByRoutePolicy(
                route_type.value, route_info)
        except Exception as e:
            raise e

    def select_node_by_route_policy(self, route_type: RouteType, dst_inst: str, dst_component: str, dst_node: str = None) -> str:
        node_list = self.select_node_list_by_route_policy(
            route_type, dst_inst, dst_component, dst_node)
        if node_list is None or len(node_list) == 0:
            return None
        selected_node_idx = random.randint(0, len(node_list) - 1)
        return node_list[selected_node_idx]

    def push_by_nodeid(self, topic: str, dstNode: bytes, seq: int, payload: bytes, timeout: int):
        route_info = self.__route_info_builder.build(
            topic=topic, dst_node=dstNode, dst_inst=None, component=None)
        result = self._push_msg(
            RouteType.ROUTE_THROUGH_NODEID.value, route_info, payload, seq, timeout)
        Transport.check_result("push_by_nodeid", result)

    def push_by_topic(self, topic: str, dstInst: str, seq: int, payload: bytes, timeout: int):
        route_info = self.__route_info_builder.build(
            topic=topic, dst_node=None, dst_inst=dstInst, component=None)
        result = self._push_msg(
            RouteType.ROUTE_THROUGH_TOPIC.value, route_info, payload, seq, timeout)
        Transport.check_result("push_by_topic", result)

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

    def get_alive_entrypoints(self, service_name: str) -> [EntryPointInfo]:
        alive_entrypoints = []
        alive_node_list = self.__transport.getFront().getNodeDiscovery().getAliveNodeList()
        if alive_node_list is None or len(alive_node_list) == 0:
            return alive_entrypoints
        for i in range(0, len(alive_node_list)):
            alive_node_info = alive_node_list[i]
            service_meta = ServiceMeta.decode(alive_node_info.meta())
            if service_meta is None or service_meta.serviceInfos is None or len(service_meta.serviceInfos) == 0:
                continue
            for entrypoint_info in service_meta.serviceInfos:
                if entrypoint_info.serviceName.lower() == service_name.lower():
                    alive_entrypoints.append(entrypoint_info)
        return alive_entrypoints

    def register_service_info(self, service, entrypoint):
        self.service_meta.serviceInfos.append(EntryPointInfo(
            service_name=service, entrypoint=entrypoint))
        self.__transport.getFront().updateMetaInfo(self.service_meta.encode())


def signal_handler(sig, frame):
    print('You pressed Ctrl+C! Exiting gracefully.')
    Transport.should_exit = True


signal.signal(signal.SIGINT, signal_handler)
