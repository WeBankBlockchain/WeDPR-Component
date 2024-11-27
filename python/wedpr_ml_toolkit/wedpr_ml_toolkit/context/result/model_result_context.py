import os
import numpy as np

from ppc_common.ppc_utils import utils
from wedpr_ml_toolkit.context.result.result_context import ResultContext
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.common.utils.constant import Constant
from wedpr_ml_toolkit.context.job_context import JobContext


class ModelResultContext(ResultContext):
    def __init__(self, job_context: JobContext, job_id: str, storage_entrypoint: StorageEntryPoint):
        super().__init__(job_context, job_id)
        self.storage_entrypoint = storage_entrypoint


class SecureLGBMResultContext(ModelResultContext):

    def __init__(self, job_context: JobContext, job_id: str, storage_entrypoint: StorageEntryPoint):
        super().__init__(job_context, job_id, storage_entrypoint)

    def parse_result(self):

        # train_praba, test_praba, train_y, test_y, feature_importance, split_xbin, trees, params
        # 从hdfs读取结果文件信息，构造为属性
        train_output = self.storage_entrypoint.download(
            self.train_model_output_file_path)
        test_output = self.storage_entrypoint.download(
            self.test_model_output_file_path)
        self.train_praba = train_output['class_pred'].values
        self.test_praba = test_output['class_pred'].values
        if 'class_label' in train_output.columns:
            self.train_y = train_output['class_label'].values
            self.test_y = test_output['class_label'].values
        else:
            self.train_y = None
            self.test_y = None

        feature_bin_data = self.storage_entrypoint.download_data(
            self.feature_bin_file_path)
        model_data = self.storage_entrypoint.download_data(self.model_path)

        self.feature_importance = ...
        self.split_xbin = feature_bin_data
        self.trees = model_data
        self.params = ...
