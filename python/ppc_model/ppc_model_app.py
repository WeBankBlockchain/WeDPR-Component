# Note: here can't be refactored by autopep
import sys
import os
root_path = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(root_path, "../"))
# Note: here can't be refactored by autopep

from ppc_model.secure_lgbm.secure_lgbm_training_engine import SecureLGBMTrainingEngine
from ppc_model.secure_lgbm.secure_lgbm_prediction_engine import SecureLGBMPredictionEngine
from ppc_model.secure_lr.secure_lr_training_engine import SecureLRTrainingEngine
from ppc_model.secure_lr.secure_lr_prediction_engine import SecureLRPredictionEngine
from ppc_model.preprocessing.preprocessing_engine import PreprocessingEngine
from ppc_model.network.http.restx import api
from ppc_model.network.http.model_controller import ns2 as log_namespace
from ppc_model.network.http.model_controller import ns as task_namespace
from ppc_model.feature_engineering.feature_engineering_engine import FeatureEngineeringEngine
from ppc_model.common.protocol import ModelTask
from ppc_model.common.global_context import components
from paste.translogger import TransLogger
from flask import Flask, Blueprint
from cheroot.wsgi import Server as WSGIServer
from cheroot.ssl.builtin import BuiltinSSLAdapter
import multiprocessing


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
    # register clear handlers
    task_manager.register_task_clear_handler(
        components.model_router.on_task_finish)

if __name__ == '__main__':
    initialize_app(app)
    register_task_handler()

    # 启动子进程不继承父进程的锁状态，防止死锁
    multiprocessing.set_start_method('spawn')

    app.config['SECRET_KEY'] = os.urandom(24)
    server = WSGIServer((app.config['HOST'], app.config['HTTP_PORT']),
                        TransLogger(app, setup_console_handler=False), numthreads=2)

    protocol = 'http'
    message = f"Starting ppc model server at {protocol}://{app.config['HOST']}:{app.config['HTTP_PORT']} successfully"
    print(message)
    components.logger().info(message)
    server.start()
    # stop the nodes
    components.transport.stop()
    print("Stop ppc model server successfully")
