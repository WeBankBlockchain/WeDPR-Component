import time
import unittest

from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.common.protocol import ModelTask
from ppc_model.common.mock.rpc_client_mock import RpcClientMock
from ppc_model.task.task_manager import TaskManager
from ppc_model.network.stub import ModelStub, PushRequest, PullRequest

rpc_client = RpcClientMock()
thread_event_manager = ThreadEventManager()
stub = ModelStub(
    agency_id='TEST_AGENCY',
    thread_event_manager=thread_event_manager,
    rpc_client=rpc_client,
    send_retry_times=3,
    retry_interval_s=0.1
)
rpc_client.set_message_handler(stub.on_message_received)


def my_send_task(args):
    print("start my_send_task")
    time.sleep(1)
    byte_array = bytearray(31 * 1024 * 1024)
    bytes_data = bytes(byte_array)
    stub.push(PushRequest(
        receiver=args['receiver'],
        task_id=args['task_id'],
        key=args['key'],
        data=bytes_data
    ))
    time.sleep(1)


def my_receive_task(args):
    print("start my_receive_task")
    stub.pull(PullRequest(
        sender=args['sender'],
        task_id=args['task_id'],
        key=args['key'],
    ))
    time.sleep(1)
    print("finish my_receive_task")


def my_failed_task(args):
    print("start my_failed_task")
    time.sleep(1)
    raise Exception('For Test')


def my_long_task(args):
    print("start my_long_task")
    stub.pull(PullRequest(
        sender=args['sender'],
        task_id=args['task_id'],
        key='not_ready',
    ))
    print("finish my_receive_task")


def my_timeout_task(args):
    print("start my_timeout_task")
    stub.pull(PullRequest(
        sender=args['sender'],
        task_id=args['task_id'],
        key='not_ready',
    ))
    print("finish my_timeout_task")


class TestTaskManager(unittest.TestCase):

    def setUp(self):
        self._task_manager = TaskManager(
            logger=MockLogger(),
            thread_event_manager=thread_event_manager,
            stub=stub,
            task_timeout_h=0.0005
        )
        self._task_manager.register_task_handler(
            ModelTask.FEATURE_ENGINEERING, my_send_task)
        self._task_manager.register_task_handler(
            ModelTask.PREPROCESSING, my_receive_task)
        self._task_manager.register_task_handler(
            ModelTask.XGB_TRAINING, my_failed_task)
        self._task_manager.register_task_handler(
            ModelTask.XGB_PREDICTING, my_long_task)

    def test_run_task(self):
        args = {
            'receiver': 'TEST_AGENCY',
            'sender': 'TEST_AGENCY',
            'task_id': '0x12345678',
            'job_id': '0x123456789',
            'key': 'TEST_MESSAGE',
        }
        self._task_manager.run_task(
            "my_send_task", ModelTask.FEATURE_ENGINEERING, (args,))
        self.assertEqual(self._task_manager.status(
            "my_send_task")[0], 'RUNNING')
        self._task_manager.run_task(
            "my_receive_task", ModelTask.PREPROCESSING, (args,))
        self.assertEqual(self._task_manager.status(
            "my_receive_task")[0], 'RUNNING')
        self._task_manager.run_task(
            "my_failed_task", ModelTask.XGB_TRAINING, (args,))
        self.assertEqual(self._task_manager.status(
            "my_failed_task")[0], 'RUNNING')
        time.sleep(3)
        self.assertEqual(self._task_manager.status(
            "my_send_task")[0], 'COMPLETED')
        self.assertEqual(self._task_manager.status(
            "my_receive_task")[0], 'COMPLETED')
        self.assertEqual(self._task_manager.status(
            "my_failed_task")[0], 'FAILED')
        time.sleep(1)

    def test_kill_task(self):
        args = {
            'receiver': 'TEST_AGENCY',
            'sender': 'TEST_AGENCY',
            'task_id': 'my_long_task',
            'job_id': '0x123456789',
            'key': 'TEST_MESSAGE',
        }
        self._task_manager.run_task("my_long_task", ModelTask.XGB_PREDICTING, (args,))
        self.assertEqual(self._task_manager.status("my_long_task")[0], 'RUNNING')
        self._task_manager.kill_task("0x123456789")
        time.sleep(1)
        self.assertEqual(self._task_manager.status(
            "my_long_task")[0], 'FAILED')

        self._task_manager.register_task_handler(
            ModelTask.XGB_PREDICTING, my_timeout_task)
        args = {
            'receiver': 'TEST_AGENCY',
            'sender': 'TEST_AGENCY',
            'task_id': 'my_timeout_task',
            'job_id': '0x123456789',
            'key': 'TEST_MESSAGE',
        }
        self._task_manager.run_task(
            "my_timeout_task", ModelTask.XGB_PREDICTING, (args,))
        self.assertEqual(self._task_manager.status(
            "my_timeout_task")[0], 'RUNNING')
        time.sleep(6)
        self.assertEqual(self._task_manager.status(
            "my_timeout_task")[0], 'FAILED')


if __name__ == '__main__':
    unittest.main()
