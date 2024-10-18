# -*- coding: utf-8 -*-

from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import FrontConfig
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import EndPoint
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import GrpcConfig


class TransportConfig:
    """
    the transport config
    """

    def __init__(self, threadpool_size: int, nodeID: str, gateway_targets: str):
        self.__transport_builder = TransportBuilder()
        self.__front_config = self.__transport_builder.buildConfig(
            threadpool_size, nodeID)
        self.__front_config.setGatewayGrpcTarget(gateway_targets)

    def get_front_config(self) -> FrontConfig:
        return self.__front_config

    def set_self_endpoint(self, host: str, port: int, listen_ip: str = None):
        endPoint = EndPoint(host, port)
        if listen_ip is None:
            listen_ip = "0.0.0.0"
        endPoint.setListenIp(listen_ip)
        self.__front_config.setSelfEndPoint(endPoint)

    def set_grpc_config(self, grpc_config: GrpcConfig):
        self.__front_config.set_grpc_config(grpc_config)

    def get_thread_pool_size(self) -> int:
        return self.__front_config.threadPoolSize()

    def get_node_id(self) -> str:
        return self.__front_config.nodeID()

    def get_gateway_targets(self) -> str:
        return self.__front_config.gatewayGrpcTarget()

    def get_self_endpoint(self) -> EndPoint:
        return self.__front_config.selfEndPoint()

    def desc(self):
        return f"thread_pool_size: {self.get_thread_pool_size()}, \
                nodeID: {self.get_node_id()}, \
                gatewayTargets: {self.get_gateway_targets()}, \
                endPoint: {self.get_self_endpoint().entryPoint()}"
