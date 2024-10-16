# Note: here can't be refactored by autopep
import sys
sys.path.append("../")

import os
from concurrent import futures
from threading import Thread
import grpc
from ppc_common.ppc_protos.generated import ppc_model_pb2_grpc
from ppc_common.ppc_utils import utils
from ppc_model_gateway import config
from ppc_model_gateway.endpoints.node_to_partner import NodeToPartnerService
from ppc_model_gateway.endpoints.partner_to_node import PartnerToNodeService



log = config.get_logger()


def node_to_partner_serve():
    rpc_port = config.CONFIG_DATA['NODE_TO_PARTNER_RPC_PORT']

    ppc_serve = grpc.server(futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1)),
                            options=config.grpc_options)
    ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(
        NodeToPartnerService(), ppc_serve)
    address = "[::]:{}".format(rpc_port)
    ppc_serve.add_insecure_port(address)

    ppc_serve.start()

    start_message = f'Start ppc model gateway internal rpc server at {rpc_port}'
    print(start_message)
    log.info(start_message)
    ppc_serve.wait_for_termination()


def partner_to_node_serve():
    rpc_port = config.CONFIG_DATA['PARTNER_TO_NODE_RPC_PORT']

    if config.CONFIG_DATA['SSL_SWITCH'] == 0:
        ppc_serve = grpc.server(futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1)),
                                options=config.grpc_options)
        ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(
            PartnerToNodeService(), ppc_serve)
        address = "[::]:{}".format(rpc_port)
        ppc_serve.add_insecure_port(address)
    else:
        grpc_root_crt = utils.load_credential_from_file(
            os.path.abspath(config.CONFIG_DATA['SSL_CA']))
        grpc_ssl_key = utils.load_credential_from_file(
            os.path.abspath(config.CONFIG_DATA['SSL_KEY']))
        grpc_ssl_crt = utils.load_credential_from_file(
            os.path.abspath(config.CONFIG_DATA['SSL_CRT']))
        server_credentials = grpc.ssl_server_credentials(((
            grpc_ssl_key,
            grpc_ssl_crt,
        ),), grpc_root_crt, True)

        ppc_serve = grpc.server(futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1)),
                                options=config.grpc_options)
        ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(
            PartnerToNodeService(), ppc_serve)
        address = "[::]:{}".format(rpc_port)
        ppc_serve.add_secure_port(address, server_credentials)

    ppc_serve.start()

    start_message = f'Start ppc model gateway external rpc server at {rpc_port}'
    print(start_message)
    log.info(start_message)
    ppc_serve.wait_for_termination()


if __name__ == '__main__':
    log = config.get_logger()

    # 设置守护线程
    node_to_partner_serve_thread = Thread(target=node_to_partner_serve)
    partner_to_node_serve_thread = Thread(target=partner_to_node_serve)

    node_to_partner_serve_thread.daemon = True
    partner_to_node_serve_thread.daemon = True

    node_to_partner_serve_thread.start()
    partner_to_node_serve_thread.start()

    node_to_partner_serve_thread.join()
    partner_to_node_serve_thread.join()

    message = f'Start ppc model gateway successfully.'
    print(message)
    log.info(message)
