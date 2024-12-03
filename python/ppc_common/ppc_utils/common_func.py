# -*- coding: utf-8 -*-
from contextlib import contextmanager
import chardet


def get_config_value(key, default_value, config_value, required):
    if required and config_value is None:
        raise Exception(f"Invalid config for '{key}' for not set!")
    value = config_value
    if type(config_value) is dict:
        if key in config_value:
            value = config_value[key]
        else:
            value = default_value
    if value is None:
        return default_value
    return value


def get_file_encoding(file_path):
    encoding = None
    with open(file_path, "rb") as fp:
        header = fp.readline()
        file_chardet = chardet.detect(header)
        if file_chardet is None:
            raise Exception(f"Unknown File Encoding, file: {file_path}")
        encoding = file_chardet["encoding"]
    return encoding


def require_non_empty(value_property, value):
    if value is None or len(value) == 0:
        raise Exception(f"the ${value_property} must non-empty!")
