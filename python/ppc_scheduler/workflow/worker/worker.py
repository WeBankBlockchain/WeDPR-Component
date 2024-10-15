import time

from func_timeout import FunctionTimedOut
from prefect import Task
from prefect.engine import signals

from ppc_scheduler.common import log_utils
from ppc_scheduler.database import job_worker_mapper
from ppc_scheduler.node.node_manager import ComputingNodeManager
from ppc_scheduler.workflow.common import codec, flow_utils
from ppc_scheduler.workflow.common.worker_status import WorkerStatus
from ppc_scheduler.workflow.common.worker_type import WorkerType


class Worker(Task):
    def __init__(self, components, job_context, worker_id, worker_type, worker_args, retries=0, retry_delay_s=0, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.components = components
        self.node_manager = ComputingNodeManager(components)
        self.logger = components.logger()
        self.job_context = job_context
        self.worker_id = worker_id
        self.worker_type = worker_type
        self.worker_args = worker_args
        self.retries = retries
        self.retry_delay_s = retry_delay_s

    def log_worker(self):
        self.components.logger().info(" ## View worker, job_id = %s, work_id = %s", self.job_context.job_id, self.worker_id)

    def engine_run(self, worker_inputs) -> list:
        # this func should be implemented by subclass
        ...

    def run(self, worker_status, worker_inputs):
        self.components.logger().info("Worker begin run, job_id = %s, work_id = %s, work_status = %s", self.job_context.job_id, self.worker_id, worker_status)
        start_time = time.time()
        try:
            # job is killed
            if self.components.thread_event_manager.event_status(self.job_context.job_id):
                self._write_failed_status(WorkerStatus.KILLED)
                self.logger.warn(
                    f" worker was killed, job_id: {self.job_context.job_id}, worker_id: {self.worker_id}")
                raise signals.FAIL(message='killed!')

            if worker_status == WorkerStatus.SUCCESS:
                self.logger.info(
                    f" Worker has been executed successfully, job_id: {self.job_context.job_id}, worker_id: {self.worker_id}")
                return self._load_output_from_db()

            inputs = []
            if self.worker_type != WorkerType.T_ON_SUCCESS \
                    and self.worker_type != WorkerType.T_ON_FAILURE:
                inputs = flow_utils.to_origin_inputs(worker_inputs)
            
            outputs = self._try_run_task(inputs)
            if outputs is not None:
                self._save_worker_result(outputs)
        except FunctionTimedOut:
            self._write_failed_status(WorkerStatus.FAILURE)
            end_time = time.time()
            self.logger.error(
                f"[OnError] job worker was timeout, job_id: {self.job_context.job_id}, worker_id: {self.worker_id}, elapsed time: {end_time - start_time}")
            raise signals.FAIL(message='timeout!')
        except BaseException as be:
            self._write_failed_status(WorkerStatus.FAILURE)
            self.logger.error(f"[OnError] job worker failed, job_id: {self.job_context.job_id}, worker_id: {self.worker_id}")
            self.logger.exception(be)
            raise signals.FAIL(message='failed!')

    def _try_run_task(self, inputs):
        start_time = time.time()
        # self.logger.info(f"Worker try run task, job_id: {self.job_context.job_id}, worker_id: {self.worker_id}, inputs: {inputs}")
        # parse inputs for worker
        if self.retries:
            attempt = 0
            while attempt <= self.retries:
                try:
                    self.logger.info(log_utils.worker_start_log_info(self.worker_id))
                    outputs = self.engine_run(inputs)
                    self.logger.info(log_utils.worker_end_log_info(self.worker_id))
                    return outputs
                except Exception as e:
                    attempt += 1
                    if attempt > self.retries:
                        self.logger.error(
                            f"worker failed after {self.retries} attempts, "
                            f"job_id: {self.job_context.job_id}, worker_id: {self.worker_id}")
                        raise e
                    else:
                        self.logger.warn(
                            f"worker failed, attempts: {attempt}"
                            f"job_id: {self.job_context.job_id}, worker_id: {self.worker_id}")
                        self.logger.exception(e)
                        time.sleep(self.retry_delay_s)
        else:
            # outputs = self.engine_run(inputs)
            self.logger.info(log_utils.worker_start_log_info(self.worker_id))
            outputs = self.engine_run(inputs)
            self.logger.info(log_utils.worker_end_log_info(self.worker_id))
            end_time = time.time()
            # self.logger.info(f"Worker try run task end, job_id: {self.job_context.job_id}, worker: {self.worker_id}, elapsed time: {end_time - start_time}, outputs: {outputs}")
            return outputs

    def _load_output_from_db(self):
        with self.components.create_sql_session() as session:
            worker_record = job_worker_mapper.query_job_worker(session, self.job_context.job_id, self.worker_id)
            return codec.deserialize_worker_outputs(worker_record.outputs)

    def _save_worker_result(self, outputs):
        with self.components.create_sql_session() as session:
            job_worker_mapper.update_job_worker(session, self.job_context.job_id, self.worker_id,
                                                WorkerStatus.SUCCESS, outputs)

    def _write_failed_status(self, status):
        with self.components.create_sql_session() as session:
            job_worker_mapper.update_job_worker(session, self.job_context.job_id, self.worker_id, status, [])
