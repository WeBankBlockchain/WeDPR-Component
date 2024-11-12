import multiprocessing
import time
import unittest

# 示例用法
from ppc_common.ppc_async_executor.async_subprocess_executor import AsyncSubprocessExecutor
from ppc_common.ppc_async_executor.async_thread_executor import AsyncThreadExecutor
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_mock.mock_objects import MockLogger


def task1(shared_status1, key, dur):
    shared_status1[key] = 1
    time.sleep(dur)
    shared_status1[key] = 2


def task2(shared_status2, key, dur):
    shared_status2[key] = 1
    time.sleep(dur)
    shared_status2[key] = 2


def on_target_finish(task_id, flag, e=None):
    if flag:
        print(f'{task_id}, success')
    else:
        print(f'{task_id}, failed, {e}')


class TestSubprocessExecutor(unittest.TestCase):

    def test_kill(self):
        logger = MockLogger()
        executor = AsyncSubprocessExecutor(logger)

        # 使用 Manager 来创建共享状态字典
        manager = multiprocessing.Manager()
        shared_status1 = manager.dict()
        shared_status2 = manager.dict()

        # 启动两个任务
        key = 'test_kill'
        executor.execute('0xaa', task1, on_target_finish,
                         (shared_status1, key, 2))
        executor.execute('0xbb', task2, on_target_finish,
                         (shared_status2, key, 2))

        time.sleep(1)

        # 终止一个任务
        if executor.kill('0xaa'):
            print(f"Task {'0xaa'} has been terminated")

        # 等待一段时间
        time.sleep(3)

        self.assertEqual(shared_status1.get(key, None), 1)
        self.assertEqual(shared_status2.get(key, None), 2)


thread_event_manager = ThreadEventManager()


def thread_task1(shared_status1, task_id, key, dur):
    shared_status1[key] = 1
    while not thread_event_manager.event_status(task_id):
        time.sleep(dur)
    shared_status1[key] = 2


def thread_task2(shared_status2, key, dur):
    shared_status2[key] = 1
    time.sleep(dur)
    shared_status2[key] = 2
    raise Exception("raise error")


class TestThreadExecutor(unittest.TestCase):

    def test_kill(self):
        logger = MockLogger()
        executor = AsyncThreadExecutor(thread_event_manager, logger)

        shared_status1 = {'test': 'test'}
        shared_status2 = {'test': 'test'}
        key = 'test_kill'
        task_id_1 = '0xaa'
        task_id_2 = '0xbb'

        executor.execute("job1", task_id_1, thread_task1, on_target_finish,
                         (shared_status1, task_id_1, key, 1))
        executor.execute("job2", task_id_2, thread_task2,
                         on_target_finish, (shared_status2, key, 1))

        time.sleep(2)

        # 终止一个任务
        if executor.kill(task_id_1):
            print(f"Task {task_id_1} has been terminated")

        # 等待一段时间
        time.sleep(1)
        self.assertEqual(shared_status1.get(key, None), 2)
        self.assertEqual(shared_status2.get(key, None), 2)


if __name__ == '__main__':
    unittest.main()
