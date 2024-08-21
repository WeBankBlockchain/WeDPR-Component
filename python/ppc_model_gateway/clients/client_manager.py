import os

import grpc

from ppc_common.ppc_protos.generated.ppc_model_pb2_grpc import ModelServiceStub
from ppc_common.ppc_utils import utils
from ppc_model_gateway import config


class ClientManager:
    def __init__(self, config_data, grpc_options):
        self._config_data = config_data
        self._grpc_options = grpc_options
        channel = grpc.insecure_channel(
            self._config_data['NODE_ENDPOINT'], options=self._grpc_options)
        self.node_stub = ModelServiceStub(channel)
        self.agency_stub_dict = {}
        self._create_partner_stubs()

    def _create_partner_stubs(self):
        for agency_id, endpoint in self._get_agency_dict().items():
            if self._config_data['SSL_SWITCH'] == 0:
                channel = grpc.insecure_channel(
                    endpoint, options=self._grpc_options)
                self.agency_stub_dict[agency_id] = ModelServiceStub(channel)
            else:
                channel = self._create_secure_channel(endpoint)
                self.agency_stub_dict[agency_id] = ModelServiceStub(channel)

    def _get_agency_dict(self) -> dict:
        agency_dict = {}
        for entry in self._config_data.get('AGENCY_LIST', []):
            if ':' in entry:
                key, value = entry.split(":", 1)
                key = key.strip()
                value = value.strip()
                agency_dict[key] = value
        return agency_dict

    def _create_secure_channel(self, target):
        grpc_root_crt = utils.load_credential_from_file(
            os.path.abspath(self._config_data['SSL_CA']))
        grpc_ssl_key = utils.load_credential_from_file(
            os.path.abspath(self._config_data['SSL_KEY']))
        grpc_ssl_crt = utils.load_credential_from_file(
            os.path.abspath(self._config_data['SSL_CRT']))

        credentials = grpc.ssl_channel_credentials(
            root_certificates=grpc_root_crt,
            private_key=grpc_ssl_key,
            certificate_chain=grpc_ssl_crt
        )

        return grpc.secure_channel(target, credentials, options=self._grpc_options)


client_manager = ClientManager(config.CONFIG_DATA, config.grpc_options)
