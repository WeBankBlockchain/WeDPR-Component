# -*- coding: utf-8 -*-
from enum import Enum
import pandas as pd


class EvaluationType(Enum):
    """the evaluation type
    """
    TRAIN = "train",
    VALIDATION = "validation",


class EvaluationMetric:
    """the evaluation metric
    """

    def __init__(self, value, desc):
        self.value = value
        self.desc = desc

    def set_desc(self, desc):
        self.desc = desc


class FeatureEvaluationResult:
    DEFAULT_SAMPLE_STAT_DESC = "总样本"
    DEFAULT_POSITIVE_SAMPLE_STAT_DESC = "正样本"
    DEFAULT_KS_STAT_DESC = "KS"
    DEFAULT_AUC_STAT_DESC = "AUC"

    DEFAULT_TRAIN_EVALUATION_DESC = "训练集"
    DEFAULT_VALIDATION_EVALUATION_DESC = "验证集"
    DEFAULT_ROW_INDEX_LABEL_DESC = "分类"

    def __init__(self, type, type_desc=None, ks_value=0, auc_value=0, label_list=None):
        self.type = type
        self.type_desc = type_desc
        if self.type_desc is None:
            if self.type == EvaluationType.TRAIN:
                self.type_desc = FeatureEvaluationResult.DEFAULT_TRAIN_EVALUATION_DESC
            elif self.type == EvaluationType.VALIDATION:
                self.type_desc = FeatureEvaluationResult.DEFAULT_VALIDATION_EVALUATION_DESC
            else:
                raise Exception(
                    f"Create FeatureEvaluationResult for unsupported evaluation type: {type}")
        self.ks = EvaluationMetric(
            ks_value, FeatureEvaluationResult.DEFAULT_KS_STAT_DESC)
        self.auc = EvaluationMetric(
            auc_value, FeatureEvaluationResult.DEFAULT_AUC_STAT_DESC)
        self.positive_samples = EvaluationMetric(
            0, FeatureEvaluationResult.DEFAULT_POSITIVE_SAMPLE_STAT_DESC)
        self.samples = EvaluationMetric(
            0, FeatureEvaluationResult.DEFAULT_SAMPLE_STAT_DESC)

        if label_list is not None:
            self.set_sample_info(label_list)

    def set_sample_info(self, label_list):
        self.samples.value = len(label_list)
        for label in label_list:
            self.positive_samples.value += label

    def columns(self):
        return [FeatureEvaluationResult.DEFAULT_ROW_INDEX_LABEL_DESC, self.samples.desc, self.positive_samples.desc, self.ks.desc, self.auc.desc]

    def to_dict(self):
        return {FeatureEvaluationResult.DEFAULT_ROW_INDEX_LABEL_DESC: self.type_desc,
                self.samples.desc: self.samples.value,
                self.positive_samples.desc: self.positive_samples.value,
                self.ks.desc: self.ks.value,
                self.auc.desc: self.auc.value}

    @staticmethod
    def summary(evaluation_result_list):
        if evaluation_result_list is None or len(evaluation_result_list) == 0:
            return None
        columns = None
        rows = []
        for evaluation_metric in evaluation_result_list:
            rows.append(evaluation_metric.to_dict())
            if columns is None:
                columns = evaluation_metric.columns()
        return pd.DataFrame(rows, columns=columns)

    @staticmethod
    def store_and_upload_summary(evaluation_result_list, local_file_path, remote_file_path, storage_client, user=None):
        df = FeatureEvaluationResult.summary(evaluation_result_list)
        df.to_csv(local_file_path, index=False)
        if storage_client is not None:
            storage_client.upload_file(local_file_path, remote_file_path, user)
