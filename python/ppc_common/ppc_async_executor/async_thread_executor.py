import threading
import time
import traceback
from typing import Callable

from ppc_common.ppc_async_executor.async_executor import AsyncExecutor
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager


class AsyncThreadExecutor(AsyncExecutor):
    def __init__(self, event_manager: ThreadEventManager, logger):
        self.event_manager = event_manager
        self.logger = logger
        self.threads = {}
        self.lock = threading.Lock()
        self._cleanup_thread = threading.Thread(target=self._loop_cleanup)
        self._cleanup_thread.daemon = True
        self._cleanup_thread.start()

    def execute(self, task_id: str, target: Callable, on_target_finish: Callable[[str, bool, Exception], None],
                args=()):
        def thread_target(logger, on_finish, *args):
            try:
                target(*args)
                on_finish(task_id, True)
            except Exception as e:
                logger.warn(traceback.format_exc())
                on_finish(task_id, False, e)

        thread = threading.Thread(target=thread_target, args=(
            self.logger, on_target_finish,) + args)
        thread.start()

        with self.lock:
            self.threads[task_id] = thread

        stop_event = threading.Event()
        self.event_manager.add_event(task_id, stop_event)

    def kill(self, task_id: str):
        with self.lock:
            if task_id not in self.threads:
                return False
            else:
                thread = self.threads[task_id]

        self.event_manager.set_event(task_id)
        thread.join()
        self.logger.info(f"Task {task_id} has been stopped!")
        return True

    def kill_all(self):
        with self.lock:
            keys = self.threads.keys()

        for task_id in keys:
            self.kill(task_id)

    def _loop_cleanup(self):
        while True:
            self._cleanup_finished_threads()
            time.sleep(3)

    def _cleanup_finished_threads(self):
        with self.lock:
            finished_threads = [
                task_id for task_id, thread in self.threads.items() if not thread.is_alive()]

        for task_id in finished_threads:
            with self.lock:
                del self.threads[task_id]
            self.logger.info(f"Cleanup finished task thread {task_id}")

    def __del__(self):
        self.kill_all()
