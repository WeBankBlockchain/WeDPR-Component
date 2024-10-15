# -*- coding: utf-8 -*-
import logging
import logging.config
import os

import yaml

path = os.getcwd()
log_dir = os.sep.join([path, 'logs'])
chain_log_dir = os.sep.join([path, 'bin', 'logs'])
print(f"log_dir: {log_dir}")
print(f"chain_log_dir: {chain_log_dir}")
if not os.path.exists(log_dir):
    os.makedirs(log_dir)
if not os.path.exists(chain_log_dir):
    os.makedirs(chain_log_dir)
logging_conf_path = os.path.normpath('logging.conf')
logging.config.fileConfig(logging_conf_path)


def get_logger(name=None):
    log = logging.getLogger(name)
    return log


config_path = "application.yml"

CONFIG_DATA = {}
agency_dict = {}


def read_config():
    with open(config_path, 'rb') as f:
        global CONFIG_DATA
        CONFIG_DATA = yaml.safe_load(f.read())


read_config()

grpc_options = [
    ('grpc.ssl_target_name_override', 'PPCS MODEL GATEWAY'),
    ('grpc.max_send_message_length',
     CONFIG_DATA['MAX_MESSAGE_LENGTH_MB'] * 1024 * 1024),
    ('grpc.max_receive_message_length',
     CONFIG_DATA['MAX_MESSAGE_LENGTH_MB'] * 1024 * 1024),
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
     '{ "retryPolicy":{ "maxAttempts": 4, "initialBackoff": "0.1s", "maxBackoff": "1s", "backoffMutiplier": 2, "retryableStatusCodes": [ "UNAVAILABLE" ] } }')
]
