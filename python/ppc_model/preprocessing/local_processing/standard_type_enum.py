from enum import Enum, unique


@unique
class standardType(Enum):
    min_max = "min-max"
    z_score = "z-score"
