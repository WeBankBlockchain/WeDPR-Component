import logging
import logging.config
import os
import threading

import yaml

from ppc_common.deps_services import storage_loader
from ppc_common.ppc_utils import common_func
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from wedpr_python_gateway_sdk.transport.impl.transport_loader import TransportLoader
from ppc_model.network.wedpr_model_transport import ModelRouter
from ppc_common.deps_services.mysql_storage import MySQLStorage
from ppc_common.ppc_config.sql_storage_config_loader import SQLStorageConfigLoader
from ppc_model.network.wedpr_model_transport import ModelTransport
from ppc_model.task.task_manager import TaskManager
from ppc_model.task.task_manager import TaskPersistent
from ppc_model.log.log_retriever import LogRetriever


class Initializer:
    MODEL_SERVICE_NAME = "MODEL"

    def __init__(self, log_config_path, config_path, plot_lock=None):
        self.log_config_path = log_config_path
        logging.config.fileConfig(self.log_config_path)
        self.config_path = config_path
        self.config_data = None
        # 只用于测试
        self.mock_logger = None
        self.public_key_length = 2048
        self.homo_algorithm = 0
        self.init_config()
        self.job_cache_dir = common_func.get_config_value(
            "JOB_TEMP_DIR", "/tmp", self.config_data, False)
        self.thread_event_manager = ThreadEventManager()

        task_persistent = TaskPersistent(self.logger(), MySQLStorage(
            storage_config=SQLStorageConfigLoader.load(self.config_data)))

        self.storage_client = storage_loader.load(
            self.config_data, self.logger())
        log_path = common_func.get_config_value(
            "LOG_PATH", "logs/wedpr-model.log", self.config_data, False)
        self.log_retriever = LogRetriever(
            self.logger(), self.job_cache_dir, self.storage_client, log_path)
        self.task_manager = TaskManager(
            logger=self.logger(),
            task_persistent=task_persistent,
            log_retriever=self.log_retriever,
            thread_event_manager=self.thread_event_manager,
            task_timeout_h=self.config_data['TASK_TIMEOUT_H']
        )
        # default send msg timeout
        self.MODEL_COMPONENT = "WEDPR_MODEL"
        self.send_msg_timeout_ms = 5000
        self.pop_msg_timeout_ms = 60000
        # for UT
        self.transport = None
        self.model_router = None
        # matplotlib 线程不安全，并行任务绘图增加全局锁
        self.plot_lock = plot_lock
        if plot_lock is None:
            self.plot_lock = threading.Lock()

    def init_config(self):
        with open(self.config_path, 'rb') as f:
            self.config_data = yaml.safe_load(f.read())
            self.public_key_length = self.config_data['PUBLIC_KEY_LENGTH']
            storage_type = common_func.get_config_value(
                "STORAGE_TYPE", "HDFS", self.config_data, False)
            if 'HOMO_ALGORITHM' in self.config_data:
                self.homo_algorithm = self.config_data['HOMO_ALGORITHM']

    def init_all(self):
        agency_id = common_func.get_config_value(
            "AGENCY_ID", None, self.config_data, True)
        self.init_transport(task_manager=self.task_manager,
                            self_agency_id=agency_id,
                            component_type=self.MODEL_COMPONENT,
                            send_msg_timeout_ms=self.send_msg_timeout_ms,
                            pop_msg_timeout_ms=self.pop_msg_timeout_ms)

    def init_transport(self, task_manager: TaskManager,
                       self_agency_id: str,
                       component_type: str,
                       send_msg_timeout_ms: int,
                       pop_msg_timeout_ms: int):
        # create the transport
        transport_config = TransportLoader.build_config(**self.config_data)
        listen_port = self.config_data["HTTP_PORT"]
        access_entrypoint = f"{transport_config.get_self_endpoint().host()}:{listen_port}"
        if not access_entrypoint.startswith("http://"):
            access_entrypoint = f"http://{access_entrypoint}"
        # register the access_entrypoint information
        transport_config.register_service_info(
            Initializer.MODEL_SERVICE_NAME, access_entrypoint)
        transport = TransportLoader.load(transport_config)
        self.logger(
            f"Create transport success, config: {transport.get_config().desc()}, access_entrypoint: {access_entrypoint}")
        # start the transport
        transport.start()
        self.logger().info(
            f"Start transport success, config: {transport.get_config().desc()}")
        self.transport = ModelTransport(transport=transport,
                                        self_agency_id=self_agency_id,
                                        task_manager=task_manager,
                                        component_type=component_type,
                                        send_msg_timeout_ms=send_msg_timeout_ms,
                                        pop_msg_timeout_ms=pop_msg_timeout_ms)
        self.model_router = ModelRouter(logger=self.logger(),
                                        transport=self.transport)

    def logger(self, name=None):
        if self.mock_logger is None:
            return logging.getLogger(name)
        else:
            return self.mock_logger
