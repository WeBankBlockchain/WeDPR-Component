from abc import ABC
from typing import Callable


class AsyncExecutor(ABC):
    def execute(self,
                job_id: str,
                task_id: str,
                target: Callable,
                on_target_finish: Callable[[str, bool, Exception], None],
                args=()):
        """
        启动一个新进程/线程执行指定的目标函数。
        :param task_id: 任务id
        :param target: 目标函数
        :param on_target_finish: 目标函数函数退出后的回调
        :param args: 函数的参数元组
        """
        pass

    def kill(self, task_id: str) -> bool:
        """
        强制终止目标函数。
        :param task_id: 任务id
        :return 是否成功
        """
        pass

    def kill_all(self):
        """
        强制终止所有目标函数。
        """
        pass

    def __del__(self):
        self.kill_all()
