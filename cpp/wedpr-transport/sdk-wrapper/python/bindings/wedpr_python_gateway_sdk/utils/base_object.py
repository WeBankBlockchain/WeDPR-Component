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
