import logging
import logging.config
import os

import yaml

from ppc_common.deps_services import storage_loader
from ppc_common.deps_services.storage_api import StorageType
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_utils import common_func
from ppc_model.network.grpc.grpc_client import GrpcClient
from ppc_model.network.stub import ModelStub
from ppc_model.task.task_manager import TaskManager


class Initializer:
    def __init__(self, log_config_path, config_path):
        self.log_config_path = log_config_path
        self.config_path = config_path
        self.config_data = None
        self.grpc_options = None
        self.stub = None
        self.task_manager = None
        self.thread_event_manager = None
        self.storage_client = None
        # 只用于测试
        self.mock_logger = None
        self.public_key_length = 2048
        self.homo_algorithm = 0

    def init_all(self):
        self.init_log()
        self.init_config()
        self.init_stub()
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

    def init_stub(self):
        self.thread_event_manager = ThreadEventManager()
        self.grpc_options = [
            ('grpc.ssl_target_name_override', 'PPCS MODEL GATEWAY'),
            ('grpc.max_send_message_length',
             self.config_data['MAX_MESSAGE_LENGTH_MB'] * 1024 * 1024),
            ('grpc.max_receive_message_length',
             self.config_data['MAX_MESSAGE_LENGTH_MB'] * 1024 * 1024),
            ('grpc.keepalive_time_ms', 15000),  # 每 15 秒发送一次心跳
            ('grpc.keepalive_timeout_ms', 5000),  # 等待心跳回应的超时时间为 5 秒
            ('grpc.keepalive_permit_without_calls', True),  # 即使没有调用也允许发送心跳
            ('grpc.http2.min_time_between_pings_ms', 15000),  # 心跳之间最小时间间隔为 15 秒
            ('grpc.http2.max_pings_without_data', 0),  # 在发送数据前不限制心跳次数
            # 在没有数据传输的情况下，确保心跳包之间至少有 20 秒的间隔
            ('grpc.http2.min_ping_interval_without_data_ms', 20000),
            ("grpc.so_reuseport", 1),
            ("grpc.use_local_subchannel_pool", 1),
            ('grpc.enable_retries', 1),
            ('grpc.service_config',
             '{ "retryPolicy":{ "maxAttempts": 4, "initialBackoff": "0.1s", "maxBackoff": "1s", "backoffMutiplier": '
             '2, "retryableStatusCodes": [ "UNAVAILABLE" ] } }')
        ]
        rpc_client = GrpcClient(
            logger=self.logger(),
            endpoint=self.config_data['GATEWAY_ENDPOINT'],
            grpc_options=self.grpc_options,
            ssl_switch=self.config_data['SSL_SWITCH'],
            ca_path=self.config_data['CA_CRT'],
            ssl_key_path=self.config_data['SSL_KEY'],
            ssl_crt_path=self.config_data['SSL_CRT']
        )
        self.stub = ModelStub(
            agency_id=self.config_data['AGENCY_ID'],
            thread_event_manager=self.thread_event_manager,
            rpc_client=rpc_client
        )

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
