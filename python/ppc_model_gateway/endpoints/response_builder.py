from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelResponse


def build_error_model_response(message: str):
    model_response = ModelResponse()
    model_response.base_response.error_code = -1
    model_response.base_response.message = message
    return model_response
