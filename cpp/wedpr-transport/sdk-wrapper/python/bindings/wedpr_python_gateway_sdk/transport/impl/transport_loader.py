# -*- coding: utf-8 -*-
from wedpr_python_gateway_sdk.utils.base_object import BaseObject
from wedpr_python_gateway_sdk.transport.impl.transport_config import TransportConfig
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.impl.transport import Transport
from wedpr_python_gateway_sdk.utils import utils
import signal


class TransportConfigObject(BaseObject):
    DEFAULT_THREADPOOL_SIZE = 4
    DEFAULT_LISTEN_IP = "0.0.0.0"

    def __init__(self, transport_threadpool_size: int = DEFAULT_THREADPOOL_SIZE,
                 transport_node_id: str = None,
                 transport_gateway_targets: str = None,
                 transport_host_ip: str = None,
                 transport_listen_port: int = None,
                 transport_listen_ip: str = DEFAULT_LISTEN_IP,
                 **params):
        self.transport_threadpool_size = transport_threadpool_size
        self.transport_node_id = transport_node_id
        self.transport_gateway_targets = transport_gateway_targets
        self.transport_host_ip = transport_host_ip
        self.transport_listen_port = transport_listen_port
        self.transport_listen_ip = transport_listen_ip
        self.set_params(**params)
        self.__check()

    def __check(self):
        if self.transport_threadpool_size <= 0:
            self.transport_threadpool_size = TransportConfigObject.DEFAULT_THREADPOOL_SIZE
        utils.require_not_empty("transport_node_id", self.transport_node_id)
        utils.require_not_empty(
            "transport_gateway_targets", self.transport_gateway_targets)
        utils.require_not_empty("transport_host_ip", self.transport_host_ip)
        if self.transport_listen_port <= 0 or self.transport_listen_port > 65535:
            raise Exception(
                f"Invalid transport listen port: {self.transport_listen_port}, must between 0-65535!")

    def __repr__(self):
        return f"transport_threadpool_size: {self.transport_threadpool_size}, " \
               f"transport_node_id: {self.transport_node_id}, " \
               f"transport_gateway_targets: {self.transport_gateway_targets}, " \
               f"transport_host_ip: {self.transport_host_ip}, transport_listen_port: {self.transport_listen_port}"


class TransportLoader:
    transport_builder = TransportBuilder()
    transport_builder.initLog("conf/wedpr_sdk_log_config.ini")

    @staticmethod
    def load(transport_config: TransportConfig) -> Transport:
        transport = TransportLoader.transport_builder.buildProTransport(
            transport_config.get_front_config())
        return Transport(transport, transport_config)

    @staticmethod
    def build_config(transport_threadpool_size: int = 4,
                     transport_node_id: str = None,
                     transport_gateway_targets: str = None,
                     transport_host_ip: str = None,
                     transport_listen_port: int = None,
                     **params) -> TransportConfig:
        config = TransportConfigObject(transport_threadpool_size,
                                       transport_node_id,
                                       transport_gateway_targets,
                                       transport_host_ip,
                                       transport_listen_port, **params)
        # build the transport config
        transport_config = TransportConfig(config.transport_threadpool_size,
                                           config.transport_node_id,
                                           config.transport_gateway_targets)
        # set the self-endpoint
        transport_config.set_self_endpoint(config.transport_host_ip,
                                           config.transport_listen_port,
                                           config.transport_listen_ip)
        return transport_config

    @staticmethod
    def build(transport_threadpool_size: int = 4,
              transport_node_id: str = None,
              transport_gateway_targets: str = None,
              transport_host_ip: str = None,
              transport_listen_port: int = None,
              **params):
        transport_config = TransportLoader.build_config(transport_threadpool_size,
                                                        transport_node_id,
                                                        transport_gateway_targets,
                                                        transport_host_ip,
                                                        transport_listen_port,
                                                        **params)
        return TransportLoader.load(transport_config)
