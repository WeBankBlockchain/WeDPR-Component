# -*- coding: utf-8 -*-
import uuid
from enum import Enum
import random
from common.utils.constant import Constant
from urllib.parse import urlencode, urlparse, parse_qs, quote

class IdPrefixEnum(Enum):
    DATASET = "d-"
    ALGORITHM = "a-"
    JOB = "j-"


def make_id(prefix):
    return prefix + str(uuid.uuid4()).replace("-", "")

def generate_nonce(nonce_len):
    return ''.join(random.choice(Constant.NUMERIC_ARRAY) for _ in range(nonce_len))

def add_params_to_url(url, params):
    parsed_url = urlparse(url)
    query_params = parse_qs(parsed_url.query)
    for key, value in params.items():
        query_params[key] = value
    new_query = urlencode(query_params, doseq=True)
    return parsed_url._replace(query=new_query).geturl()
