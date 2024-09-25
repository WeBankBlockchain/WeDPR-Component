import random
import time

from ppc_common.ppc_utils import http_utils
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode


class PWSApi:
    def __init__(self, endpoint, token, 
                 polling_interval_s: int = 5, max_retries: int = 5, retry_delay_s: int = 5):
        self.endpoint = endpoint
        self.token = token
        self.polling_interval_s = polling_interval_s
        self.max_retries = max_retries
        self.retry_delay_s = retry_delay_s
        self._async_run_task_method = 'asyncRunTask'
        self._get_task_status_method = 'getTaskStatus'
        self._completed_status = 'COMPLETED'
        self._failed_status = 'FAILED'

    def run(self, datasets, params):
        params = {
            'jsonrpc': '1',
            'method': self._async_run_task_method,
            'token': self.token,
            'id': random.randint(1, 65535),
            'dataset': datasets,
            'params': params
        }
        response = self._send_request_with_retry(http_utils.send_post_request, self.endpoint, None, params)
        if response.status_code != 200:
            raise Exception(f"创建任务失败: {response.json()}")
        return self._poll_task_status(response.job_id, self.token)

    def _poll_task_status(self, job_id, token):
        while True:
            params = {
                'jsonrpc': '1',
                'method': self._get_task_status_method,
                'token': token,
                'id': random.randint(1, 65535),
                'params': {
                    'taskID': job_id,
                }
            }
            response = self._send_request_with_retry(http_utils.send_post_request, self.endpoint, None, params)
            if response.status_code != 200:
                raise Exception(f"轮询任务失败: {response.json()}")
            if response['result']['status'] == self._completed_status:
                return response['result']
            elif response['result']['status'] == self._failed_status:
                raise PpcException(PpcErrorCode.CALL_SCS_ERROR.get_code(), response['data'])
            time.sleep(self.polling_interval_s)

    def _send_request_with_retry(self, request_func, *args, **kwargs):
        attempt = 0
        while attempt < self.max_retries:
            try:
                response = request_func(*args, **kwargs)
                return response
            except Exception as e:
                attempt += 1
                if attempt < self.max_retries:
                    time.sleep(self.retry_delay_s)
                else:
                    raise e
