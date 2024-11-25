# -*- coding: utf-8 -*-
import uuid
from enum import Enum
import shutil
import os
import random
from wedpr_ml_toolkit.common.utils.constant import Constant


class IdPrefixEnum(Enum):
    DATASET = "d-"
    ALGORITHM = "a-"
    JOB = "j-"


def make_id(prefix):
    return prefix + str(uuid.uuid4()).replace("-", "")


def generate_nonce(nonce_len):
    return ''.join(str(random.choice(Constant.NUMERIC_ARRAY)) for _ in range(nonce_len))


def file_exists(_file):
    if os.path.exists(_file) and os.path.isfile(_file):
        return True
    return False


def delete_file(path):
    if os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path)
    else:
        raise (Exception(' path not exisited ! path => %s', path))


def get_config_value(key, default_value, config_value, required):
    if required and config_value is None:
        raise Exception(f"Invalid config for '{key}' for not set!")
    if config_value is None:
        return default_value
    value = config_value
    if type(config_value) is dict:
        if key in config_value:
            value = config_value[key]
        else:
            value = default_value
    if value is None:
        return default_value
    return value
