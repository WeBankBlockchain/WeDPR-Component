# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.common.utils.constant import Constant
from abc import abstractmethod
import os


class ResultContext:
    def __init__(self, job_context: JobContext, job_id: str, user: str):
        self.job_id = job_id
        self.job_context = job_context
        # init the result file path
        self.model_base_dir = os.path.join(
            self.job_context.user_config.user, "share", "jobs", "model", self.job_id)
        self.feature_bin_file_path = os.path.join(
            self.model_base_dir, Constant.FEATURE_BIN_FILE)
        self.test_model_output_file_path = os.path.join(
            self.model_base_dir, Constant.TEST_MODEL_OUTPUT_FILE)
        self.train_model_output_file_path = os.path.join(
            self.model_base_dir, Constant.TRAIN_MODEL_OUTPUT_FILE)
        self.fe_result_file_path = os.path.join(
            self.model_base_dir, Constant.FE_RESULT_FILE)
        self.model_path = os.path.join(
            self.model_base_dir, Constant.MODEL_DATA_FILE)
        self.psi_result_file_path = os.path.join(
            self.job_context.user_config.user, "share", "psi", f"psi-{self.job_id}")
        self.parse_result()

    @abstractmethod
    def parse_result(self):
        pass
