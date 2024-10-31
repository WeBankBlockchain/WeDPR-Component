from abc import ABC


class TaskEngine(ABC):
    task_type: str

    @staticmethod
    def run(task_id: str, args: dict):
        ...
