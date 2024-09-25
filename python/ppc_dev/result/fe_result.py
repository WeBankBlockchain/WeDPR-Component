import os

from ppc_dev.wedpr_data.data_context import DataContext
from ppc_dev.common.base_result import BaseResult


class FeResult(BaseResult):

    FE_RESULT_FILE = "fe_result.csv"

    def __init__(self, dataset: DataContext, job_id: str):

        super().__init__(dataset.ctx)
        self.job_id = job_id
        
        participant_id_list = []
        for dataset in self.dataset.datasets:
            participant_id_list.append(dataset.agency.agency_id)
        self.participant_id_list = participant_id_list

        result_list = []
        for dataset in self.dataset.datasets:
            dataset.update_path(os.path.join(self.job_id, self.FE_RESULT_FILE))
            result_list.append(dataset)

        fe_result = DataContext(*result_list)
        return fe_result
