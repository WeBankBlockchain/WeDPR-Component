import datetime
import logging
import os
import threading
import time
from typing import Callable, Union

from readerwriterlock import rwlock

from ppc_common.ppc_async_executor.async_thread_executor import AsyncThreadExecutor
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_model.common.protocol import ModelTask, TaskStatus, LOG_START_FLAG_FORMATTER, LOG_END_FLAG_FORMATTER
from ppc_model.network.stub import ModelStub


class TaskManager:
    def __init__(self, logger,
                 thread_event_manager: ThreadEventManager,
                 stub: ModelStub,
                 task_timeout_h: Union[int, float]):
        self.logger = logger
        self._thread_event_manager = thread_event_manager
        self._stub = stub
        self._task_timeout_s = task_timeout_h * 3600
        self._rw_lock = rwlock.RWLockWrite()
        self._tasks: dict[str, list] = {}
        self._jobs: dict[str, set] = {}
        self._handlers = {}
        self._async_executor = AsyncThreadExecutor(
            event_manager=self._thread_event_manager, logger=logger)
        self._cleanup_thread = threading.Thread(target=self._loop_cleanup)
        self._cleanup_thread.daemon = True
        self._cleanup_thread.start()

    def register_task_handler(self, task_type: ModelTask, task_handler: Callable):
        """
        注册任务的执行入口
        param task_type: 任务类型
        param task_handler: 任务执行入口
        """
        self._handlers[task_type.value] = task_handler

    def run_task(self, task_id: str, task_type: ModelTask, args=()):
        """
        发起任务
        param task_id: 任务ID
        param task_type: 任务类型
        param args: 各任务参数
        """
        job_id = args[0]['job_id']
        with self._rw_lock.gen_wlock():
            if task_id in self._tasks:
                self.logger.info(
                    f"Task already exists, task_id: {task_id}, status: {self._tasks[task_id][0]}")
                return
            self._tasks[task_id] = [TaskStatus.RUNNING.value,
                                    datetime.datetime.now(), 0, args[0]['job_id']]
            if job_id in self._jobs:
                self._jobs[job_id].add(task_id)
            else:
                self._jobs[job_id] = {task_id}
        self.logger.info(LOG_START_FLAG_FORMATTER.format(job_id=job_id))
        self.logger.info(f"Run task, job_id: {job_id}, task_id: {task_id}")
        self._async_executor.execute(
            task_id, self._handlers[task_type.value], self._on_task_finish, args)

    def kill_task(self, job_id: str):
        """
        终止任务
        """
        task_ids = []
        with self._rw_lock.gen_rlock():
            if job_id not in self._jobs:
                return
            for task_id in self._jobs[job_id]:
                task_ids.append(task_id)

        for task_id in task_ids:
            self.kill_one_task(task_id)

    def kill_one_task(self, task_id: str):
        with self._rw_lock.gen_rlock():
            if task_id not in self._tasks or self._tasks[task_id][0] != TaskStatus.RUNNING.value:
                return

        self.logger.info(f"Kill task, task_id: {task_id}")
        self._async_executor.kill(task_id)

        with self._rw_lock.gen_wlock():
            self._tasks[task_id][0] = TaskStatus.FAILED.value

    def status(self, task_id: str) -> [str, float, float]:
        """
        返回: 任务状态, 通讯量(MB), 执行耗时(s)
        """
        with self._rw_lock.gen_rlock():
            if task_id not in self._tasks:
                raise PpcException(
                    PpcErrorCode.TASK_NOT_FOUND.get_code(),
                    PpcErrorCode.TASK_NOT_FOUND.get_msg())
            status = self._tasks[task_id][0]
            traffic_volume = self._stub.traffic_volume(task_id)
            time_costs = self._tasks[task_id][2]
            return status, traffic_volume, time_costs

    def _on_task_finish(self, task_id: str, is_succeeded: bool, e: Exception = None):
        with self._rw_lock.gen_wlock():
            time_costs = (datetime.datetime.now() -
                          self._tasks[task_id][1]).total_seconds()
            self._tasks[task_id][2] = time_costs
            if is_succeeded:
                self._tasks[task_id][0] = TaskStatus.COMPLETED.value
                self.logger.info(f"Task {task_id} completed, job_id: {self._tasks[task_id][3]}, "
                                 f"time_costs: {time_costs}s")
            else:
                self._tasks[task_id][0] = TaskStatus.FAILED.value
                self.logger.warn(f"Task {task_id} failed, job_id: {self._tasks[task_id][3]}, "
                                 f"time_costs: {time_costs}s, error: {e}")
            self.logger.info(LOG_END_FLAG_FORMATTER.format(
                job_id=self._tasks[task_id][3]))

    def _loop_cleanup(self):
        while True:
            self._terminate_timeout_tasks()
            self._cleanup_finished_tasks()
            time.sleep(5)

    def _terminate_timeout_tasks(self):
        tasks_to_kill = []
        with self._rw_lock.gen_rlock():
            for task_id, value in self._tasks.items():
                alive_time = (datetime.datetime.now() -
                              value[1]).total_seconds()
                if alive_time >= self._task_timeout_s and value[0] == TaskStatus.RUNNING.value:
                    tasks_to_kill.append(task_id)

        for task_id in tasks_to_kill:
            self.logger.warn(f"Task is timeout, task_id: {task_id}")
            self.kill_one_task(task_id)

    def _cleanup_finished_tasks(self):
        tasks_to_cleanup = []
        with self._rw_lock.gen_rlock():
            for task_id, value in self._tasks.items():
                alive_time = (datetime.datetime.now() -
                              value[1]).total_seconds()
                if alive_time >= self._task_timeout_s + 3600:
                    tasks_to_cleanup.append((task_id, value[3]))
        with self._rw_lock.gen_wlock():
            for task_id, job_id in tasks_to_cleanup:
                if task_id in self._tasks:
                    del self._tasks[task_id]
                if job_id in self._jobs:
                    del self._jobs[job_id]
                self._thread_event_manager.remove_event(task_id)
                self._stub.cleanup_cache(task_id)
                self.logger.info(
                    f"Cleanup task cache, task_id: {task_id}, job_id: {job_id}")

    def record_model_job_log(self, job_id):
        log_file = self._get_log_file_path()
        if log_file is None or log_file == "":
            current_working_dir = os.getcwd()
            relative_log_path = "logs/wedpr-model.log"
            log_file = os.path.join(current_working_dir, relative_log_path)

        start_keyword = LOG_START_FLAG_FORMATTER.format(job_id=job_id)
        end_keyword = LOG_END_FLAG_FORMATTER.format(job_id=job_id)
        with open(log_file, 'r') as file:
            log_data = file.read()
        start_index = log_data.find(start_keyword)
        end_index = log_data.rfind(end_keyword)

        if start_index == -1 or end_index == -1:
            return f"{job_id} not found in log data"

        end_index += len(end_keyword)
        return log_data[start_index:end_index]

    def _get_log_file_path(self):
        log_file_path = None
        for handler in self.logger.handlers:
            if isinstance(handler, logging.FileHandler):
                log_file_path = handler.baseFilename
                break
        return log_file_path
