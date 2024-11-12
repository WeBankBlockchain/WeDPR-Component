# -*- coding: utf-8 -*-

import json
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import FrontConfig
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import EndPoint
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import GrpcConfig
from wedpr_python_gateway_sdk.transport.api.transport_api import ServiceMeta
from wedpr_python_gateway_sdk.transport.api.transport_api import EntryPointInfo


class TransportConfig:
    """
    the transport config
    """

    def __init__(self, threadpool_size: int, nodeID: str, gateway_targets: str):
        self.__transport_builder = TransportBuilder()
        self.__front_config = self.__transport_builder.buildConfig(
            threadpool_size, nodeID)
        self.__front_config.setGatewayGrpcTarget(gateway_targets)
        self.service_meta = ServiceMeta(entrypoints_info=[])

    def get_front_config(self) -> FrontConfig:
        return self.__front_config

    def register_service_info(self, service, entrypoint):
        self.service_meta.serviceInfos.append(EntryPointInfo(
            service_name=service, entrypoint=entrypoint))
        self.__front_config.setMeta(self.service_meta.encode())

    def register_service_meta(self, service_meta: ServiceMeta):
        if service_meta is None:
            return
        self.service_meta = service_meta
        self.__front_config.setMeta(self.service_meta.encode())

    def get_service_meta(self) -> ServiceMeta:
        return self.service_meta

    def set_self_endpoint(self, host: str, port: int, listen_ip: str = None):
        endpoint = EndPoint(host, port)
        if listen_ip is None:
            listen_ip = "0.0.0.0"
        endpoint.setListenIp(listen_ip)
        self.__front_config.setSelfEndPoint(endpoint)

    def set_grpc_config(self, grpc_config: GrpcConfig):
        self.__front_config.setGrpcConfig(grpc_config)

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
                endPoint: {self.get_self_endpoint().entryPoint()}," \
               f"serviceMeta: {self.service_meta}"
