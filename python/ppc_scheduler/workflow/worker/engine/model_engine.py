import time

from ppc_scheduler.workflow.common.job_context import JobContext
from ppc_scheduler.workflow.common.worker_type import WorkerType
from ppc_scheduler.workflow.worker.engine.work_engine import WorkerEngine


class ModelWorkerEngine(WorkerEngine):
    def __init__(self, model_client, worker_type, worker_id, components, job_context: JobContext):
        self.model_client = model_client
        self.worker_type = worker_type
        self.worker_id = worker_id
        self.components = components
        self.job_context = job_context
        self.logger = self.components.logger()

    def run(self, *args) -> list:
        if self.worker_type == WorkerType.T_MODEL:
            pass
        else:
            raise ValueError(f"Unsupported worker type: {self.worker_type}")

        job_id = self.job_context.job_id
        start_time = time.time()

        self.logger.info(
            f"## model engine run begin, job_id={job_id}, worker_id={self.worker_id}, args: {args}")

        # send job request to model node and wait for the job to finish
        self.model_client.run(*args)

        time_costs = time.time() - start_time
        self.logger.info(
            f"## model engine run finished, job_id={job_id}, timecost: {time_costs}s")

        # args = {
        #     'job_id': job_id,
        #     'task_id': task_id,
        #     'task_type': 'PREPROCESSING',
        #     'dataset_id': self.job_context.dataset_id,
        #     'dataset_storage_path': dataset_storage_path,
        #     'job_algorithm_type': self.job_context.job_type,
        #     'need_run_psi': self.job_context.need_run_psi,
        #     'model_dict': self.job_context.model_config_dict
        # }
        # self.log.info(f"start prepare_xgb, job_id: {job_id}, task_id: {task_id}, args: {args}")
        # self.model_client.run(args)
        # self.log.info(
        #     f"call compute_xgb_job service success, job: {job_id}, "
        #     f"task_id: {task_id}, timecost: {time.time() - start}")

        return []
