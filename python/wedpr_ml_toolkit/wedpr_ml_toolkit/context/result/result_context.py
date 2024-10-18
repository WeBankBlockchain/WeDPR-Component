# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.context.job_context import JobContext
from abc import abstractmethod


class ResultContext:
    def __init__(self, job_context: JobContext, job_id: str):
        self.job_id = job_id
        self.job_context = job_context
        self.parse_result()

    @abstractmethod
    def parse_result(self):
        pass
