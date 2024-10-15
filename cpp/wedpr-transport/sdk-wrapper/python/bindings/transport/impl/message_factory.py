from transport.impl.message_impl import MessageImpl
from transport.api.message_api import MessageAPI
from transport.generated.wedpr_python_transport import Message


class MessageFactory:
    @staticmethod
    def build(message: Message) -> MessageAPI:
        if message is None:
            return None
        return MessageImpl(message)
