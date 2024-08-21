from ppc_common.ppc_protos.generated import ppc_model_pb2_grpc
from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest, ModelResponse
from ppc_model.common.global_context import components


class ModelService(ppc_model_pb2_grpc.ModelServiceServicer):

    def MessageInteraction(self, model_request: ModelRequest, context):
        components.logger().info(
            f"receive a package, sender: {model_request.sender}, task_id: {model_request.task_id}, "
            f"key: {model_request.key}, seq: {model_request.seq}, slice_num: {model_request.slice_num}")

        components.stub.on_message_received(model_request)
        model_response = ModelResponse()
        model_response.base_response.error_code = 0
        model_response.base_response.message = 'success'
        return model_response
