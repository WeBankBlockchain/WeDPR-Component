from wedpr_python_gateway_sdk.transport.impl.message_impl import MessageImpl
from wedpr_python_gateway_sdk.transport.api.message_api import MessageAPI
from wedpr_python_gateway_sdk.transport.generated.wedpr_python_transport import Message


class MessageFactory:
    @staticmethod
    def build(message: Message) -> MessageAPI:
        if message is None:
            return None
        return MessageImpl(message)
