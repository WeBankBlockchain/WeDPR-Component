import threading
from typing import Dict

from readerwriterlock import rwlock


class ThreadEventManager:
    def __init__(self):
        # Event清理由TaskManager完成
        self.events: Dict[str, threading.Event] = {}
        self.rw_lock = rwlock.RWLockWrite()

    def add_event(self, task_id: str, event: threading.Event) -> None:
        with self.rw_lock.gen_wlock():
            self.events[task_id] = event

    def remove_event(self, task_id: str):
        with self.rw_lock.gen_wlock():
            if task_id in self.events:
                del self.events[task_id]

    def set_event(self, task_id: str):
        with self.rw_lock.gen_wlock():
            if task_id in self.events:
                self.events[task_id].set()
            else:
                raise KeyError(f"Task ID {task_id} not found")

    def event_status(self, task_id: str) -> bool:
        with self.rw_lock.gen_rlock():
            if task_id in self.events:
                return self.events[task_id].is_set()
            else:
                return False
