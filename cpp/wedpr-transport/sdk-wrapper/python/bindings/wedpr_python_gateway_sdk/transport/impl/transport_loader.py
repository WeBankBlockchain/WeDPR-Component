# -*- coding: utf-8 -*-
from wedpr_python_gateway_sdk.transport.impl.transport_config import TransportConfig
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import TransportBuilder
from wedpr_python_gateway_sdk.transport.impl.transport import Transport
import signal


class TransportLoader:
    transport_builder = TransportBuilder()

    @staticmethod
    def load(transport_config: TransportConfig) -> Transport:
        transport = TransportLoader.transport_builder.buildProTransport(
            transport_config.get_front_config())
        return Transport(transport)
