import datetime
import threading
import time
from typing import Union

from readerwriterlock import rwlock

from ppc_common.ppc_async_executor.async_thread_executor import AsyncThreadExecutor
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_scheduler.common import log_utils
from ppc_scheduler.job.job_status import JobStatus
from ppc_scheduler.workflow.scheduler.scheduler import Scheduler
from ppc_scheduler.workflow.builder.flow_builder import FlowBuilder
from ppc_scheduler.workflow.common.job_context import JobContext


class JobManager:
    def __init__(self, logger,
                 scheduler: Scheduler,
                 thread_event_manager: ThreadEventManager,
                 workspace: str,
                 job_timeout_h: Union[int, float]):
        self.logger = logger
        self._thread_event_manager = thread_event_manager
        self._scheduler = scheduler
        self._workspace = workspace
        self._job_timeout_s = job_timeout_h * 3600
        self._rw_lock = rwlock.RWLockWrite()
        self._jobs: dict[str, list] = {}
        self._flow_builder = FlowBuilder(logger=logger)
        self._async_executor = AsyncThreadExecutor(
            event_manager=self._thread_event_manager, logger=logger)
        self._cleanup_thread = threading.Thread(target=self._loop_action)
        self._cleanup_thread.daemon = True
        self._cleanup_thread.start()

    def run_task(self, job_id, request_body):
        """
        run task
        param job_id: job id
        param request: job params
        """
        # TODO: The database persists job information
        with self._rw_lock.gen_wlock():
            if job_id in self._jobs:
                self.logger.info(
                    f"Job already exists, job_id: {job_id}, status: {self._jobs[job_id][0]}")
                return
            self._jobs[job_id] = [
                JobStatus.RUNNING, datetime.datetime.now(), 0]
        self.logger.info(log_utils.job_start_log_info(job_id))

        # Create job context
        job_context = JobContext.create_job_context(
            request_body, self._workspace)
        # Build job workflow
        flow_context = self._flow_builder.build_flow_context(
            job_id=job_context.job_id, workflow_configs=job_context.workflow_configs)
        # Persistent workflow
        self._flow_builder.save_flow_context(job_context.job_id, flow_context)
        # Run workflow
        self._async_executor.execute(
            job_id, self._run_job_flow, self._on_task_finish, (job_context, flow_context))

    def _run_job_flow(self, job_context, flow_context):
        """
        run job flow
        """

        # the scheduler module starts scheduling tasks
        self._scheduler.run(job_context, flow_context)

    def kill_job(self, job_id: str):
        with self._rw_lock.gen_rlock():
            if job_id not in self._jobs or self._jobs[job_id][0] != JobStatus.RUNNING:
                return

        self.logger.info(f"Kill job, job_id: {job_id}")
        self._async_executor.kill(job_id)

        with self._rw_lock.gen_wlock():
            self._jobs[job_id][0] = JobStatus.FAILURE

    def status(self, job_id: str) -> tuple[str, float]:
        """
        query task status
        return: task status and task cost(s)
        """
        with self._rw_lock.gen_rlock():
            if job_id not in self._jobs:
                raise PpcException(
                    PpcErrorCode.JOB_NOT_FOUND.get_code(),
                    PpcErrorCode.JOB_NOT_FOUND.get_msg())
            status = self._jobs[job_id][0]
            time_costs = self._jobs[job_id][2]
            return status, time_costs

    def _on_task_finish(self, job_id: str, is_succeeded: bool, e: Exception = None):
        with self._rw_lock.gen_wlock():
            time_costs = (datetime.datetime.now() -
                          self._jobs[job_id][1]).total_seconds()
            self._jobs[job_id][2] = time_costs
            if is_succeeded:
                self._jobs[job_id][0] = JobStatus.SUCCESS
                self.logger.info(
                    f"Job {job_id} completed, time_costs: {time_costs}s")
            else:
                self._jobs[job_id][0] = JobStatus.FAILURE
                self.logger.warn(
                    f"Job {job_id} failed, time_costs: {time_costs}s, error: {e}")
            self.logger.info(log_utils.job_end_log_info(job_id))

    def _loop_action(self):
        while True:
            time.sleep(20)
            self._terminate_timeout_jobs()
            # TODO: store into the database
            # self._cleanup_finished_jobs()
            self._report_jobs()

    def _terminate_timeout_jobs(self):
        jobs_to_kill = []
        with self._rw_lock.gen_rlock():
            for job_id, value in self._jobs.items():
                alive_time = (datetime.datetime.now() -
                              value[1]).total_seconds()
                if alive_time >= self._job_timeout_s and value[0] == JobStatus.RUNNING:
                    jobs_to_kill.append(job_id)

        for job_id in jobs_to_kill:
            self.logger.warn(f"Job is timeout, job_id: {job_id}")
            self.kill_job(job_id)

    def _cleanup_finished_jobs(self):
        jobs_to_cleanup = []
        with self._rw_lock.gen_rlock():
            for job_id, value in self._jobs.items():
                alive_time = (datetime.datetime.now() -
                              value[1]).total_seconds()
                if alive_time >= self._job_timeout_s + 3600:
                    jobs_to_cleanup.append((job_id, value[3]))
                    self.logger.info(f"Job is finished, job_id: {job_id}")
        with self._rw_lock.gen_wlock():
            for job_id, job_id in jobs_to_cleanup:
                if job_id in self._jobs:
                    del self._jobs[job_id]
                self._thread_event_manager.remove_event(job_id)
                self.logger.info(f"Cleanup job cache, job_id: {job_id}")

    def _report_jobs(self):
        with self._rw_lock.gen_rlock():
            job_count = len(self._jobs)
            self.logger.info(f" ## Report job count: {job_count}")
