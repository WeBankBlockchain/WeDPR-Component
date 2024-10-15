import os
import time

from ppc_common.ppc_utils import utils
from ppc_scheduler.common import log_utils
from ppc_scheduler.workflow.common.worker_type import WorkerType
from ppc_scheduler.workflow.worker.worker import Worker


class DefaultWorker(Worker):

    def __init__(self, components, job_context, worker_id, worker_type, worker_args, *args, **kwargs):
        super().__init__(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)

    def engine_run(self, worker_inputs):
        # log_utils.upload_job_log(self.components.storage_client, self.job_context.job_id)
        # self._save_workflow_view_file()
        if self.worker_type == WorkerType.T_ON_FAILURE:
            # notice job manager that this job has failed
            raise Exception("Job run failed, job_id: " + self.job_context.job_id)

    # TODO: 
    # def _save_workflow_view_file(self):
    #     file = f"{self.job_context.workflow_view_path}.svg"
        
    #     if not utils.file_exists(file):
    #         return
        
    #     try_count = 10
    #     while try_count > 0:
    #         if utils.file_exists(file):
    #             break
    #         time.sleep(1)
    #         try_count -= 1
    #    self.components.storage_client.upload_file(file,
    #                                                self.job_context.job_id + os.sep +
    #                                                self.job_context.workflow_view_path)
