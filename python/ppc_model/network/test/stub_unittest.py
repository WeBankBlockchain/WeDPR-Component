import unittest

from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_model.common.mock.rpc_client_mock import RpcClientMock
from ppc_model.network.stub import ModelStub, PushRequest, PullRequest


class TestStub(unittest.TestCase):
    def setUp(self):
        super().__init__()
        self._agency_id = 'TEST_AGENCY'
        self._message_type = 'TEST_MESSAGE'
        self._rpc_client = RpcClientMock()
        self._thread_event_manager = ThreadEventManager()
        self._stub = ModelStub(
            agency_id=self._agency_id,
            thread_event_manager=self._thread_event_manager,
            rpc_client=self._rpc_client,
            send_retry_times=3,
            retry_interval_s=0.1
        )
        self._rpc_client.set_message_handler(self._stub.on_message_received)

    def test_push_pull(self):
        task_id = '0x12345678'
        byte_array = bytearray(31 * 1024 * 1024)
        bytes_data = bytes(byte_array)
        self._stub.push(PushRequest(
            receiver=self._agency_id,
            task_id=task_id,
            key=self._message_type,
            data=bytes_data
        ))
        self._stub.push(PushRequest(
            receiver=self._agency_id,
            task_id=task_id,
            key=self._message_type + 'other',
            data=bytes_data
        ))
        received_data = self._stub.pull(PullRequest(
            sender=self._agency_id,
            task_id=task_id,
            key=self._message_type
        ))
        other_data = self._stub.pull(PullRequest(
            sender=self._agency_id,
            task_id=task_id,
            key=self._message_type + 'other',
        ))
        self.assertEqual(bytes_data, received_data)
        self.assertEqual(bytes_data, other_data)

    def test_bad_client(self):
        rpc_client = RpcClientMock(need_failed=True)
        stub = ModelStub(
            agency_id=self._agency_id,
            thread_event_manager=self._thread_event_manager,
            rpc_client=rpc_client,
            send_retry_times=3,
            retry_interval_s=0.1
        )
        rpc_client.set_message_handler(stub.on_message_received)

        task_id = '0x12345678'
        byte_array = bytearray(3 * 1024 * 1024)
        bytes_data = bytes(byte_array)
        stub.push(PushRequest(
            receiver=self._agency_id,
            task_id=task_id,
            key=self._message_type,
            data=bytes_data
        ))

        received_data = stub.pull(PullRequest(
            sender=self._agency_id,
            task_id=task_id,
            key=self._message_type
        ))
        self.assertEqual(bytes_data, received_data)


if __name__ == '__main__':
    unittest.main()
