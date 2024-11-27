# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.common.utils.constant import Constant
from abc import abstractmethod
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobDetailResponse
import os
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta
from wedpr_ml_toolkit.context.data_context import DatasetContext


class ResultContext:
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        self.check_and_get_job_type(job_context, job_result_detail)
        self.job_context = job_context
        self.job_result_detail = job_result_detail
        self.job_id = self.job_result_detail.job_object.id
        # init the result file path
        self.model_base_dir = os.path.join(
            "share", "jobs", "model", self.job_id)

        self.feature_bin_file_path = os.path.join(
            self.model_base_dir, Constant.FEATURE_BIN_FILE)
        self.fe_result_file_path = os.path.join(
            self.model_base_dir, Constant.WOE_IV_FILE)
        self.model_result_path = os.path.join(
            self.model_base_dir, Constant.MODEL_RESULT_FILE)
        self.preprocessing_result_path = os.path.join(
            self.model_base_dir, Constant.PREPROCESSING_RESULT_FILE)
        self.feature_importance_result_path = os.path.join(
            self.model_base_dir, Constant.FEATURE_IMPORTANCE_FILE)
        self.evaluation_table_path = os.path.join(
            self.model_base_dir, Constant.EVALUATION_TABLE_FILE)
        self.feature_selection_file_path = os.path.join(
            self.model_base_dir, Constant.FEATURE_SELECTION_FILE)
        self.model_path = os.path.join(
            self.model_base_dir, Constant.MODEL_FILE)
        self.woe_iv_path = os.path.join(
            self.model_base_dir, Constant.WOE_IV_FILE)

        self.psi_result_file_path = os.path.join(
            self.job_context.user_config.user, "share", "psi", f"psi-{self.job_id}")

        self.parse_result()

    @staticmethod
    def check_and_get_job_type(job_context: JobContext, job_result_detail: JobDetailResponse):
        if job_context is None:
            raise Exception(
                "Invalid empty job_context!")
        if job_result_detail is None:
            raise Exception("Invalid empty job result!")
        if job_result_detail.job_object is None:
            raise Exception(
                "Invalid job result: must contain the job information!")
        return job_result_detail.job_object.jobType

    def _generate_result_dataset_(self, dataset_file_path):
        dataset_meta = DatasetMeta(user=self.job_context.storage_entry_point.user_config.user,
                                   agency=self.job_context.user_config.agency_name,
                                   file_path=dataset_file_path)
        return DatasetContext(storage_entrypoint=self.job_context.storage_entry_point,
                              storage_workspace=None,
                              dataset_meta=dataset_meta)

    @abstractmethod
    def parse_result(self):
        pass
