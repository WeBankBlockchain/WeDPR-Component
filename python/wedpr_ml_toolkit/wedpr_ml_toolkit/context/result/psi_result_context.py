# -*- coding: utf-8 -*-
import os

from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.common.utils.constant import Constant
from wedpr_ml_toolkit.context.result.result_context import ResultContext


class PSIResultContext(ResultContext):

    PSI_RESULT_FILE = "psi_result.csv"

    def __init__(self, job_context: JobContext, job_id: str):
        super().__init__(job_context, job_id)

    def parse_result(self):
        result_list = []
        for dataset in self.job_context.dataset.datasets:
            dataset.update_path(os.path.join(
                self.job_id, Constant.PSI_RESULT_FILE))
            result_list.append(dataset)

        self.psi_result = DataContext(*result_list)
