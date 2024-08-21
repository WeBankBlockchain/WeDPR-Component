from abc import ABC


class RpcClient(ABC):
    rpc_type: str

    def send(self, request):
        ...
