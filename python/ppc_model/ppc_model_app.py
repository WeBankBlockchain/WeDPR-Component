# Note: here can't be refactored by autopep
import sys
sys.path.append("../")

import multiprocessing
import os
from concurrent import futures
from threading import Thread
import grpc
from cheroot.ssl.builtin import BuiltinSSLAdapter
from cheroot.wsgi import Server as WSGIServer
from flask import Flask, Blueprint
from paste.translogger import TransLogger
from ppc_common.ppc_protos.generated import ppc_model_pb2_grpc
from ppc_common.ppc_utils import utils
from ppc_model.common.global_context import components
from ppc_model.common.protocol import ModelTask
from ppc_model.feature_engineering.feature_engineering_engine import FeatureEngineeringEngine
from ppc_model.network.grpc.grpc_server import ModelService
from ppc_model.network.http.model_controller import ns as task_namespace
from ppc_model.network.http.model_controller import ns2 as log_namespace
from ppc_model.network.http.restx import api
from ppc_model.preprocessing.preprocessing_engine import PreprocessingEngine
from ppc_model.secure_lr.secure_lr_prediction_engine import SecureLRPredictionEngine
from ppc_model.secure_lr.secure_lr_training_engine import SecureLRTrainingEngine
from ppc_model.secure_lgbm.secure_lgbm_prediction_engine import SecureLGBMPredictionEngine
from ppc_model.secure_lgbm.secure_lgbm_training_engine import SecureLGBMTrainingEngine


app = Flask(__name__)


def initialize_app(app):
    # 初始化应用功能组件
    components.init_all()

    app.config.update(components.config_data)
    blueprint = Blueprint('api', __name__, url_prefix='/api')
    api.init_app(blueprint)
    api.add_namespace(task_namespace)
    api.add_namespace(log_namespace)
    app.register_blueprint(blueprint)


def register_task_handler():
    task_manager = components.task_manager
    task_manager.register_task_handler(
        ModelTask.PREPROCESSING, PreprocessingEngine.run)
    task_manager.register_task_handler(
        ModelTask.FEATURE_ENGINEERING, FeatureEngineeringEngine.run)
    task_manager.register_task_handler(
        ModelTask.XGB_TRAINING, SecureLGBMTrainingEngine.run)
    task_manager.register_task_handler(
        ModelTask.XGB_PREDICTING, SecureLGBMPredictionEngine.run)
    task_manager.register_task_handler(
        ModelTask.LR_TRAINING, SecureLRTrainingEngine.run)
    task_manager.register_task_handler(
        ModelTask.LR_PREDICTING, SecureLRPredictionEngine.run)


def model_serve():
    if app.config['SSL_SWITCH'] == 0:
        ppc_serve = grpc.server(futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1)),
                                options=components.grpc_options)
        ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(
            ModelService(), ppc_serve)
        address = "[::]:{}".format(app.config['RPC_PORT'])
        ppc_serve.add_insecure_port(address)
    else:
        grpc_root_crt = utils.load_credential_from_file(
            os.path.abspath(app.config['SSL_CA']))
        grpc_ssl_key = utils.load_credential_from_file(
            os.path.abspath(app.config['SSL_KEY']))
        grpc_ssl_crt = utils.load_credential_from_file(
            os.path.abspath(app.config['SSL_CRT']))
        server_credentials = grpc.ssl_server_credentials(((
            grpc_ssl_key,
            grpc_ssl_crt,
        ),), grpc_root_crt, True)

        ppc_serve = grpc.server(futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() - 1)),
                                options=components.grpc_options)
        ppc_model_pb2_grpc.add_ModelServiceServicer_to_server(
            ModelService(), ppc_serve)
        address = "[::]:{}".format(app.config['RPC_PORT'])
        ppc_serve.add_secure_port(address, server_credentials)

    ppc_serve.start()
    components.logger().info(
        f"Starting model grpc server at ://{app.config['HOST']}:{app.config['RPC_PORT']}")
    ppc_serve.wait_for_termination()


if __name__ == '__main__':
    initialize_app(app)
    register_task_handler()

    # 启动子进程不继承父进程的锁状态，防止死锁
    multiprocessing.set_start_method('spawn')

    Thread(target=model_serve).start()

    app.config['SECRET_KEY'] = os.urandom(24)
    server = WSGIServer((app.config['HOST'], app.config['HTTP_PORT']),
                        TransLogger(app, setup_console_handler=False), numthreads=2)

    ssl_switch = app.config['SSL_SWITCH']
    protocol = 'http'
    if ssl_switch == 1:
        protocol = 'https'
        server.ssl_adapter = BuiltinSSLAdapter(
            certificate=app.config['SSL_CRT'],
            private_key=app.config['SSL_KEY'],
            certificate_chain=app.config['CA_CRT'])

    message = f"Starting ppc model server at {protocol}://{app.config['HOST']}:{app.config['HTTP_PORT']}"
    print(message)
    components.logger().info(message)
    server.start()
