
import grpc
import sys
import os

from ppc_model_gateway import config
from ppc_common.ppc_protos.generated import ppc_model_pb2_grpc
from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest


def generate_bytes(size_in_mb):
    size_in_bytes = size_in_mb * 1024 * 1023
    return os.urandom(size_in_bytes)


def send_data():
    channel = grpc.insecure_channel(
        f'localhost:{port}', options=config.grpc_options)
    stub = ppc_model_pb2_grpc.ModelServiceStub(channel)

    request = ModelRequest()

    request.task_id = "task_id"
    request.receiver = receiver
    request.key = 'key'
    request.seq = 0
    request.slice_num = 1
    request.data = bytes(generate_bytes(
        config.CONFIG_DATA['MAX_MESSAGE_LENGTH_MB']))

    response = stub.MessageInteraction(request)
    print("Received response:", response.base_response.message)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python client.py <port> <receiver>")
        sys.exit(1)
    port = int(sys.argv[1])
    receiver = sys.argv[2]
    send_data()
