import logging
import logging.config
import os
import threading

import yaml

from ppc_common.deps_services import storage_loader
from ppc_common.ppc_utils import common_func
from wedpr_python_gateway_sdk.transport.impl.transport_loader import TransportLoader
from ppc_model.task.task_manager import TaskManager


class Initializer:
    def __init__(self, log_config_path, config_path, plot_lock=None):
        self.log_config_path = log_config_path
        self.config_path = config_path
        self.config_data = None
        self.grpc_options = None
        self.transport = None
        self.task_manager = None
        self.thread_event_manager = None
        self.storage_client = None
        # 只用于测试
        self.mock_logger = None
        self.public_key_length = 2048
        self.homo_algorithm = 0
        # matplotlib 线程不安全，并行任务绘图增加全局锁
        self.plot_lock = plot_lock
        if plot_lock is None:
            self.plot_lock = threading.Lock()

    def init_all(self):
        self.init_log()
        self.init_config()
        self.init_transport()
        self.init_task_manager()
        self.init_storage_client()
        self.init_cache()

    def init_log(self):
        logging.config.fileConfig(self.log_config_path)

    def init_cache(self):
        self.job_cache_dir = common_func.get_config_value(
            "JOB_TEMP_DIR", "/tmp", self.config_data, False)

    def init_config(self):
        with open(self.config_path, 'rb') as f:
            self.config_data = yaml.safe_load(f.read())
            self.public_key_length = self.config_data['PUBLIC_KEY_LENGTH']
            storage_type = common_func.get_config_value(
                "STORAGE_TYPE", "HDFS", self.config_data, False)
            if 'HOMO_ALGORITHM' in self.config_data:
                self.homo_algorithm = self.config_data['HOMO_ALGORITHM']

    def init_transport(self):
        # create the transport
        self.transport = TransportLoader.build(**self.config_data)
        self.logger(
            f"Create transport success, config: {self.get_config().desc()}")
        # start the transport
        self.transport.start()
        self.logger(
            f"Start transport success, config: {self.get_config().desc()}")

    def init_task_manager(self):
        self.task_manager = TaskManager(
            logger=self.logger(),
            thread_event_manager=self.thread_event_manager,
            stub=self.stub,
            task_timeout_h=self.config_data['TASK_TIMEOUT_H']
        )

    def init_storage_client(self):
        self.storage_client = storage_loader.load(
            self.config_data, self.logger())

    def logger(self, name=None):
        if self.mock_logger is None:
            return logging.getLogger(name)
        else:
            return self.mock_logger
