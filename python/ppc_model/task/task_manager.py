import datetime
import json
import logging
import os
import threading
import time
from typing import Callable, Union

from readerwriterlock import rwlock

from ppc_common.ppc_async_executor.async_thread_executor import AsyncThreadExecutor
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_common.deps_services.sql_storage_api import SQLStorageAPI
from ppc_model.common.protocol import ModelTask, TaskStatus, LOG_START_FLAG_FORMATTER, LOG_END_FLAG_FORMATTER
from ppc_model.db.job_worker_record import JobWorkerRecord
from ppc_model.log.log_retriever import LogRetriever


class TaskResult:
    def __init__(self, job_id, task_id):
        self.task_status = TaskStatus.RUNNING.value
        self.start_time = datetime.datetime.now()
        self.job_id = job_id
        self.task_id = task_id
        self.diagnosis_msg = ""
        self.exec_result = None

    def get_timecost(self):
        return (datetime.datetime.now() - self.start_time).total_seconds()

    def finalize(self):
        # generate the exec_result
        exec_result_dict = {}
        exec_result_dict.update({"timecost": self.get_timecost()})
        exec_result_dict.update({"diagnosis_msg": self.diagnosis_msg})
        self.exec_result = json.dumps(exec_result_dict)

    def __repr__(self):
        return f"job_id: {self.job_id}, task_id: {self.task_id}, " \
               f"task_status: {self.task_status}, start_time: {self.start_time}"


class TaskPersistent:
    def __init__(self, logger, sql_storage: SQLStorageAPI):
        self.logger = logger
        self.sql_storage = sql_storage

    def query_tasks(self, job_id) -> []:
        task_recorder_list = self.sql_storage.query(JobWorkerRecord,
                                                    JobWorkerRecord.job_id == job_id)
        tasks = []
        if task_recorder_list is None or task_recorder_list.count() == 0:
            return tasks
        self.logger.info(
            f"query_tasks, job_id: {job_id}, count: {task_recorder_list.count()}")
        for task in task_recorder_list:
            tasks.append(task)
        return tasks

    def query_task(self, worker_id) -> JobWorkerRecord:
        task = self.sql_storage.query(JobWorkerRecord,
                                      JobWorkerRecord.worker_id == worker_id)
        if task is None or task.count() == 0:
            return None
        return task.first()

    def job_finished(self, job_id):
        task_recorders = self.query_tasks(job_id)
        if task_recorders is None:
            return True
        for task in task_recorders:
            if task.status == TaskStatus.RUNNING.value or task.status == TaskStatus.PENDING.value:
                return False
        return True

    def on_task_finished(self, task_result: TaskResult):
        """
        :param task_result: the task result
        :return:
        """
        # query the task
        worker_recorder = self.query_task(task_result.task_id)
        if worker_recorder is None:
            self.logger.warn(
                f"TaskPersistent error, the task not found! task_result: {task_result}")
            return
        # update the task result
        worker_recorder.status = task_result.task_status
        worker_recorder.exec_result = task_result.exec_result
        self.sql_storage.merge(worker_recorder)


class TaskManager:
    def __init__(self, logger,
                 task_persistent: TaskPersistent,
                 log_retriever: LogRetriever,
                 thread_event_manager: ThreadEventManager,
                 task_timeout_h: Union[int, float]):
        self.logger = logger
        self.task_persistent = task_persistent
        self.log_retriever = log_retriever
        self._thread_event_manager = thread_event_manager
        self._task_timeout_s = task_timeout_h * 3600
        self._rw_lock = rwlock.RWLockWrite()
        self._tasks: dict[str, TaskResult] = {}
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
        task_recorder = self.task_persistent.query_task(task_id)
        if task_recorder is not None and task_recorder.status == TaskStatus.RUNNING.value:
            self.logger.info(
                f"Task already exists and Running, task_id: {task_id}")
            return
        with self._rw_lock.gen_wlock():
            if task_id in self._tasks:
                self.logger.info(
                    f"Task already exists, task_id: {task_id}, status: {self._tasks[task_id].task_id}")
                return
            self._tasks[task_id] = TaskResult(
                job_id=args[0]['job_id'], task_id=task_id)
        self.logger.info(LOG_START_FLAG_FORMATTER.format(job_id=job_id))
        self.logger.info(f"Run task, job_id: {job_id}, task_id: {task_id}")
        self._async_executor.execute(
            args[0]['job_id'], task_id, self._handlers[task_type.value], self._on_task_finish, args)

    def kill_task(self, job_id: str):
        """
        终止任务
        """
        tasks = self.task_persistent.query_tasks(job_id)
        for task in tasks:
            self.kill_one_task(task.worker_id)
        self.logger.info(LOG_END_FLAG_FORMATTER.format(
            job_id=job_id))
        # upload the log
        self.logger.info(f"kill_task, job {job_id} killed, upload the log")
        self.log_retriever.upload_log(job_id)

    def kill_one_task(self, task_id: str):
        task_result = None
        with self._rw_lock.gen_rlock():
            if task_id not in self._tasks.keys() or self._tasks[task_id].task_status != TaskStatus.RUNNING.value:
                return
            task_result = self._tasks[task_id]
        self.logger.info(f"Kill task, task_id: {task_id}")
        self._async_executor.kill(task_id)

        # persistent the status to killed
        with self._rw_lock.gen_wlock():
            task_result.task_status = TaskStatus.KILLED.value
            self.task_persistent.on_task_finished(task_result)
            self._tasks.pop(task_id)
        self.logger.info(f"Kill task success, task_id: {task_id}")

    def task_finished(self, task_id: str) -> bool:
        (status, _, _) = self.status(task_id)
        if status == TaskStatus.RUNNING.value or status == TaskStatus.PENDING.value:
            return False
        return True

    def status(self, task_id: str) -> [str, float, str]:
        """
        返回: 任务状态, 通讯量(MB), 执行耗时(s)
        """
        result = self.task_persistent.query_task(task_id)
        if result is None:
            return TaskStatus.NotFound.value, 0.0, None
        return result.status, 0, result.exec_result

    def _on_task_finish(self, task_id: str, is_succeeded: bool, e: Exception = None):
        task_result = None
        with self._rw_lock.gen_wlock():
            if task_id not in self._tasks.keys():
                self.logger.warn(
                    f"_on_task_finish: the task {task_id} not Found! maybe killed!")
                return
            task_result = self._tasks[task_id]

        # update the task result
        if is_succeeded:
            task_result.task_status = TaskStatus.SUCCESS.value
            self.logger.info(f"Task {task_id} completed, job_id: {task_result.job_id}, "
                             f"time_costs: {task_result.get_timecost()}s")
        else:
            task_result.task_status = TaskStatus.FAILURE.value
            task_result.diagnosis_msg = str(e)
            self.logger.warn(f"Task {task_id} failed, job_id: {task_result.job_id}, "
                             f"time_costs: {self._tasks[task_id].get_timecost()}s, error: {e}")

        self.logger.info(LOG_END_FLAG_FORMATTER.format(
            job_id=task_result.job_id))
        # finalize the task result
        task_result.finalize()
        self.task_persistent.on_task_finished(task_result)

        with self._rw_lock.gen_wlock():
            # erase from the queue
            self._tasks.pop(task_id)

            # record and upload the log if all task finished
        if self.task_persistent.job_finished(task_result.job_id):
            self.logger.info(
                f"_on_task_finish: all sub tasks finished, upload the log, task_info: {task_result}")
            self.log_retriever.upload_log(task_result.job_id)

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
                self._thread_event_manager.remove_event(task_id)
                self.logger.info(
                    f"Cleanup task cache, task_id: {task_id}, job_id: {job_id}")
