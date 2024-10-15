# -*- coding: utf-8 -*-
import os
import time

from ppc_common.ppc_utils import utils, common_func
from ppc_scheduler.workflow.common.job_context import JobContext
from ppc_scheduler.workflow.worker.engine.work_engine import WorkerEngine


class PsiWorkerEngine(WorkerEngine):
    def __init__(self, psi_client, worker_id, worker_type, components, job_context: JobContext):
        self.psi_client = psi_client
        self.worker_id = worker_id
        self.worker_type = worker_type
        self.components = components
        self.job_context = job_context
        self.logger = self.components.logger()

    def run(self, *args) -> list:
        job_id = self.job_context.job_id
        start_time = time.time()

        self.logger.info(f"## psi engine run begin, job_id={job_id}, worker_id={self.worker_id}, args: {args}")
        
        # send job request to psi node and wait for the job to finish
        self.psi_client.run(*args)
        
        time_costs = time.time() - start_time
        self.logger.info(f"## psi engine run finished, job_id={job_id}, timecost: {time_costs}s")
        
        return [JobContext.HDFS_STORAGE_PATH + job_id + os.sep + self.job_context.PSI_RESULT_INDEX_FILE]