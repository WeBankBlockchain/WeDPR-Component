# -*- coding: utf-8 -*-
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import MessageOptionalHeaderBuilder
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import MessageOptionalHeader


class RouteInfoBuilder:
    def __init__(self, builder: MessageOptionalHeaderBuilder):
        self.__route_info_builder = builder

    def build(self, topic: str, dst_node: bytes, dst_inst: str, component: str) -> MessageOptionalHeader:
        routeInfo = self.__route_info_builder.build()
        if topic is not None:
            routeInfo.setTopic(topic)
        if dst_node is not None:
            routeInfo.setDstNodeBuffer(dst_node)
        if dst_inst is not None:
            routeInfo.setDstInst(dst_inst)
        if component is not None:
            routeInfo.setComponentType(component)
        return routeInfo
