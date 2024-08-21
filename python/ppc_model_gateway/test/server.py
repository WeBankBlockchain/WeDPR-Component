import os
from concurrent import futures
import grpc
import sys

from ppc_model_gateway import config
from ppc_common.ppc_protos.generated import ppc_model_pb2_grpc
from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelResponse


class ModelService(ppc_model_pb2_grpc.ModelServiceServicer):
    def MessageInteraction(self, request, context):
        response = ModelResponse()
        response.base_response.error_code = 0
        response.base_response.message = "Data received successfully."
        response.data = request.data
        return response


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(
        max_workers=max(1, os.cpu_count() - 1)),
        options=config.grpc_options)
    ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(ModelService(), server)
    server.add_insecure_port(f'[::]:{port}')
    server.start()
    print(f'Start serve successfully at {port}.')
    server.wait_for_termination()


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python server.py <port>")
        sys.exit(1)
    port = sys.argv[1]
    serve()
