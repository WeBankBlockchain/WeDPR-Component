import time
import traceback

from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest
from ppc_common.ppc_protos.generated.ppc_model_pb2_grpc import ModelServiceServicer
from ppc_model_gateway import config
from ppc_model_gateway.clients.client_manager import client_manager
from ppc_model_gateway.endpoints.response_builder import build_error_model_response

log = config.get_logger()


class PartnerToNodeService(ModelServiceServicer):

    def MessageInteraction(self, request: ModelRequest, context):
        start_time = time.time()
        try:
            log.debug(
                f"start sending data to {request.receiver}, task_id: {request.task_id}, "
                f"key: {request.key}, seq: {request.seq}")
            response = client_manager.node_stub.MessageInteraction(request)
            end_time = time.time()
            log.info(
                f"finish sending data to {request.receiver}, task_id: {request.task_id}, "
                f"key: {request.key}, seq: {request.seq}, slice_num: {request.slice_num}, "
                f"ret_code: {response.base_response.error_code}, time_costs: {str(end_time - start_time)}s")
        except Exception:
            end_time = time.time()
            message = f"[OnWarn]Send data to {request.receiver} failed, task_id: {request.task_id}, " \
                      f"key: {request.key}, seq: {request.seq}, slice_num: {request.slice_num}, " \
                      f"exception:{str(traceback.format_exc())}, time_costs: {str(end_time - start_time)}s"
            log.warn(message)
            response = build_error_model_response(message)
        return response
