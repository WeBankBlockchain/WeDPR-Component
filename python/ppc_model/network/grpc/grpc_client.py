import os
import time
import traceback

import grpc

from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelRequest, ModelResponse
from ppc_common.ppc_protos.generated.ppc_model_pb2_grpc import ModelServiceStub
from ppc_common.ppc_utils import utils
from ppc_model.common.protocol import RpcType
from ppc_model.interface.rpc_client import RpcClient


class GrpcClient(RpcClient):
    rpc_type = RpcType.GRPC

    def __init__(self, logger, endpoint: str, grpc_options, ssl_switch: int = 0,
                 ca_path=None, ssl_key_path=None, ssl_crt_path=None):
        self._logger = logger
        self._endpoint = endpoint
        self._ssl_switch = ssl_switch
        self._grpc_options = grpc_options
        self._ca_path = ca_path
        self._ssl_key_path = ssl_key_path
        self._ssl_crt_path = ssl_crt_path
        if self._ssl_switch == 0:
            insecure_channel = grpc.insecure_channel(
                self._endpoint, options=grpc_options)
            self._client = ModelServiceStub(insecure_channel)
        else:
            channel = self._create_secure_channel(self._endpoint)
            self._client = ModelServiceStub(channel)

    def _create_secure_channel(self, target):
        grpc_root_crt = utils.load_credential_from_file(
            os.path.abspath(self._ca_path))
        grpc_ssl_key = utils.load_credential_from_file(
            os.path.abspath(self._ssl_key_path))
        grpc_ssl_crt = utils.load_credential_from_file(
            os.path.abspath(self._ssl_crt_path))
        credentials = grpc.ssl_channel_credentials(
            root_certificates=grpc_root_crt,
            private_key=grpc_ssl_key,
            certificate_chain=grpc_ssl_crt
        )
        return grpc.secure_channel(target, credentials, options=self._grpc_options)

    @staticmethod
    def _build_error_model_response(message: str):
        model_response = ModelResponse()
        model_response.base_response.error_code = -1
        model_response.base_response.message = message
        return model_response

    def send(self, request: ModelRequest):
        start_time = time.time()
        try:
            self._logger.debug(
                f"start sending data to {request.receiver}, task_id: {request.task_id}, "
                f"key: {request.key}, seq: {request.seq}")
            response = self._client.MessageInteraction(request)
            end_time = time.time()
            if response.base_response.error_code != 0:
                self._logger.warn(
                    f"[OnWarn]send data to {request.receiver} failed, task_id: {request.task_id}, "
                    f"key: {request.key}, seq: {request.seq}, slice_num: {request.slice_num}, "
                    f"ret_code: {response.base_response.error_code}, message: {response.base_response.message}, "
                    f"time_costs: {str(end_time - start_time)}s")
            else:
                self._logger.info(
                    f"finish sending data to {request.receiver}, task_id: {request.task_id}, "
                    f"key: {request.key}, seq: {request.seq}, slice_num: {request.slice_num}, "
                    f"ret_code: {response.base_response.error_code}, message: {response.base_response.message}, "
                    f"time_costs: {str(end_time - start_time)}s")
        except Exception:
            end_time = time.time()
            message = f"[OnWarn]Send data to {request.receiver} failed, task_id: {request.task_id}, " \
                      f"key: {request.key}, seq: {request.seq}, slice_num: {request.slice_num}, " \
                      f"exception:{str(traceback.format_exc())}, time_costs: {str(end_time - start_time)}s"
            self._logger.warn(message)
            response = self._build_error_model_response(message)
        return response
