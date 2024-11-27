import os

from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.common.utils.constant import Constant
from wedpr_ml_toolkit.context.result.result_context import ResultContext
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobDetailResponse
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta
from wedpr_ml_toolkit.context.dataset_context import DatasetContext


class PreprocessingResultContext(ResultContext):
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        super().__init__(job_context, job_result_detail)
        self.preprocessing_dataset = self._generate_result_dataset_(
            self.preprocessing_result_path)

    def __repr__(self):
        return f"preprocessing_dataset: {self.preprocessing_dataset}"


class FeResultContext(ResultContext):

    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        super().__init__(job_context, job_result_detail)
        self.fe_dataset = self._generate_result_dataset_(
            self.fe_result_file_path)

    def __repr__(self):
        return f"fe_dataset: {self.fe_dataset}"
