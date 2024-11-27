# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.context.dataset_context import DatasetContext
from wedpr_ml_toolkit.context.result.result_context import ResultContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobDetailResponse


class PSIResultContext(ResultContext):
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        super().__init__(job_context, job_result_detail)
        self.result_dataset = self._generate_result_dataset_(
            self.job_result_detail.resultFileInfo['path'])

    def __repr__(self):
        return f"result_dataset: {self.result_dataset}"
