# -*- coding: utf-8 -*-

def require_not_empty(field, value):
    if value is None or len(value) == 0:
        raise Exception(f"The field {field} must not empty!")
