import uuid
from enum import Enum


class IdPrefixEnum(Enum):
    DATASET = "d-"
    ALGORITHM = "a-"
    JOB = "j-"


def make_id(prefix):
    return prefix + str(uuid.uuid4()).replace("-", "")
