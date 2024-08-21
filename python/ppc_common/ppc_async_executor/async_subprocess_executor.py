import multiprocessing
import threading
import time
from typing import Callable

from ppc_common.ppc_async_executor.async_executor import AsyncExecutor


class AsyncSubprocessExecutor(AsyncExecutor):
    def __init__(self, logger):
        self.logger = logger
        self.processes = {}
        self.lock = threading.Lock()
        self._cleanup_thread = threading.Thread(target=self._loop_cleanup)
        self._cleanup_thread.daemon = True
        self._cleanup_thread.start()

    def execute(self, task_id: str, target: Callable, on_target_finish: Callable[[str, bool, Exception], None],
                args=()):
        process = multiprocessing.Process(target=target, args=args)
        process.start()
        with self.lock:
            self.processes[task_id] = process

    def kill(self, task_id: str):
        with self.lock:
            if task_id not in self.processes:
                return False
            else:
                process = self.processes[task_id]

        process.terminate()
        self.logger.info(f"Task {task_id} has been terminated!")
        return True

    def kill_all(self):
        with self.lock:
            keys = self.processes.keys()

        for task_id in keys:
            self.kill(task_id)

    def _loop_cleanup(self):
        while True:
            self._cleanup_finished_processes()
            time.sleep(3)

    def _cleanup_finished_processes(self):
        with self.lock:
            finished_processes = [
                (task_id, proc) for task_id, proc in self.processes.items() if not proc.is_alive()]

        for task_id, process in finished_processes:
            with self.lock:
                process.join()  # 确保进程资源释放
                del self.processes[task_id]
            self.logger.info(f"Cleanup finished task process {task_id}")

    def __del__(self):
        self.kill_all()
