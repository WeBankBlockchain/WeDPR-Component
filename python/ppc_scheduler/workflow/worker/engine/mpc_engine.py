# -*- coding: utf-8 -*-
import os
from time import time

import pandas as pd

from ppc_common.ppc_utils import utils
from ppc_scheduler.mpc_generator.generator import CodeGenerator
from ppc_scheduler.workflow.common.job_context import JobContext
from ppc_scheduler.workflow.worker.engine.work_engine import WorkerEngine


class MpcWorkerEngine(WorkerEngine):
    def __init__(self, mpc_client, worker_type, components, job_context: JobContext):
        self.mpc_client = mpc_client
        self.worker_type = worker_type
        self.components = components
        self.job_context = job_context
        self.logger = self.components.logger()

    def run(self, *args) -> list:
        
        job_id = self.job_context.job_id
        start_time = time.time()

        self.logger.info(f"computing mpc begin, job_id={job_id}")
        
        time_costs = time.time() - start_time
        self.logger.info(f"computing mpc finished, job_id={job_id}, timecost: {time_costs}s")
        
        return [self.job_context.mpc_output_path]