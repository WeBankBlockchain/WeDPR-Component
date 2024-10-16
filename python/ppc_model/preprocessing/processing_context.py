import os

from ppc_model.common.context import Context
from ppc_model.common.initializer import Initializer
from ppc_common.ppc_utils import common_func
from ppc_model.common.model_setting import ModelSetting


class ProcessingContext(Context):
    def __init__(self,
                 args,
                 components: Initializer):
        super().__init__(args['job_id'],
                         args['task_id'],
                         components,
                         role=None)
        self.dataset_path = args['dataset_path']
        self.dataset_file_path = os.path.join(
            self.workspace, args['dataset_id'])
        self.job_algorithm_type = args['algorithm_type']
        self.need_run_psi = args['need_run_psi']
        self.model_dict = args['model_dict']
        if "psi_result_path" in args:
            self.remote_psi_result_path = args["psi_result_path"]
        self.model_setting = ModelSetting(self.model_dict)
