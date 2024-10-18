# -*- coding: utf-8 -*-
from typing import Any
import time


class BaseObject:
    def set_params(self, **params: Any):
        for key, value in params.items():
            setattr(self, key, value)
            if hasattr(self, f"{key}"):
                setattr(self, f"{key}", value)
        return self

    def as_dict(obj):
        return {attr: getattr(obj, attr) for attr in dir(obj) if not callable(getattr(obj, attr)) and not attr.startswith("__")}

    def execute_with_retry(self, request_func, retry_times, retry_wait_seconds, *args, **kwargs):
        attempt = 0
        while attempt < retry_times:
            try:
                response = request_func(*args, **kwargs)
                return response
            except Exception as e:
                attempt += 1
                if attempt < retry_times:
                    time.sleep(retry_wait_seconds)
                else:
                    raise e
