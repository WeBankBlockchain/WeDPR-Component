# -*- coding: utf-8 -*-

from enum import Enum


class ClassificationType(Enum):
    TWO = 'two'
    MULTI = 'multi'

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_


class EvaluationType(Enum):
    ROC = "roc",
    PR = "pr",
    KS = "ks",
    ACCURACY = "accuracy",
    CONFUSION_MATRIX = "confusion_matrix"


class ModelRole(Enum):
    ACTIVE = "active"
    PASSIVE = "passive"
