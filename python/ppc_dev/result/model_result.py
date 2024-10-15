import os
import numpy as np

from ppc_common.ppc_utils import utils

from ppc_dev.wedpr_data.data_context import DataContext
from ppc_dev.common.base_result import BaseResult
from ppc_dev.job_exceuter.hdfs_client import HDFSApi


class ModelResult(BaseResult):

    FEATURE_BIN_FILE = "feature_bin.json"
    MODEL_DATA_FILE = utils.XGB_TREE_PERFIX + '.json'
    TEST_MODEL_OUTPUT_FILE = "test_output.csv"
    TRAIN_MODEL_OUTPUT_FILE = "train_output.csv"

    def __init__(self, dataset: DataContext, job_id: str, job_type: str):

        super().__init__(dataset.ctx)
        self.job_id = job_id

        participant_id_list = []
        for dataset in self.dataset.datasets:
            participant_id_list.append(dataset.agency.agency_id)
        self.participant_id_list = participant_id_list

        if job_type == 'xgb_training':
            self._xgb_train_result()

    def _xgb_train_result(self):

        # train_praba, test_praba, train_y, test_y, feature_importance, split_xbin, trees, params
        # 从hdfs读取结果文件信息，构造为属性
        train_praba_path = os.path.join(
            self.job_id, self.TRAIN_MODEL_OUTPUT_FILE)
        test_praba_path = os.path.join(
            self.job_id, self.TEST_MODEL_OUTPUT_FILE)
        train_output = HDFSApi.download(train_praba_path)
        test_output = HDFSApi.download(test_praba_path)
        self.train_praba = train_output['class_pred'].values
        self.test_praba = test_output['class_pred'].values
        if 'class_label' in train_output.columns:
            self.train_y = train_output['class_label'].values
            self.test_y = test_output['class_label'].values
        else:
            self.train_y = None
            self.test_y = None

        feature_bin_path = os.path.join(self.job_id, self.FEATURE_BIN_FILE)
        model_path = os.path.join(self.job_id, self.MODEL_DATA_FILE)
        feature_bin_data = HDFSApi.download_data(feature_bin_path)
        model_data = HDFSApi.download_data(model_path)

        self.feature_importance = ...
        self.split_xbin = feature_bin_data
        self.trees = model_data
        self.params = ...
