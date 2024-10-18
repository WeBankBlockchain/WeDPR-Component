from wedpr_ml_toolkit.transport.wedpr_entrypoint import WeDPREntryPoint
from wedpr_ml_toolkit.common.utils.constant import Constant
from wedpr_ml_toolkit.config.wedpr_ml_config import AuthConfig
from wedpr_ml_toolkit.config.wedpr_ml_config import JobConfig
import json
import time


class JobInfo:
    def __init__(self, job_type: str, project_name: str, param: str):
        self.jobType = job_type
        self.projectName = project_name
        self.param = param


class JobParam:
    def __init__(self, job_info: JobInfo, task_parities, dataset_list):
        self.job = job_info
        self.taskParties = task_parities
        self.datasetList = dataset_list


class WeDPRResponse:
    def __init__(self, code, msg, data):
        self.code = code
        self.msg = msg
        self.data = data

    def success(self):
        return self.code == 0


class QueryJobRequest:
    def __init__(self, job_id):
        # the job condition
        self.job = {}
        self.job.update("id", job_id)


class WeDPRRemoteJobClient(WeDPREntryPoint):
    def __init__(self, auth_config: AuthConfig, job_config: JobConfig):
        if auth_config is None:
            raise Exception("Must define the auth config!")
        if job_config is None:
            raise Exception("Must define the job config")
        super().__init__(auth_config.access_key_id, auth_config.access_key_secret,
                         auth_config.remote_entrypoints, auth_config.nonce_len)
        self.auth_config = auth_config
        self.job_config = job_config

    def get_auth_config(self):
        return self.auth_config

    def get_job_config(self):
        return self.job_config

    def submit_job(self, job_params: JobParam) -> WeDPRResponse:
        wedpr_response = self.send_post_request(
            self.job_config._submit_job_uri, None, None, json.dumps(job_params))
        submit_result = WeDPRResponse(**wedpr_response)
        # return the job_id
        if submit_result.success():
            return submit_result.data
        raise Exception(
            f"submit_job failed, code: {submit_result.code}, msg: {submit_result.msg}")

    def _poll_task_status(self, job_id, token):
        while True:
            wedpr_response = WeDPRResponse(self._send_request_with_retry(
                self.send_post_request, self.job_config.query_job_status_uri, None, None, json.dumps(QueryJobRequest(job_id))))
            # TODO: check with status
            if wedpr_response.success():
                return wedpr_response
            else:
                raise Exception(
                    f"_poll_task_status for job {job_id} failed, code: {wedpr_response.code}, msg: {wedpr_response.msg}")
            time.sleep(self.job_config.polling_interval_s)

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
