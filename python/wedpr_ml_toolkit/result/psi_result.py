import os

from wedpr_ml_toolkit.wedpr_data.data_context import DataContext
from wedpr_ml_toolkit.common.base_result import BaseResult


class PSIResult(BaseResult):

    PSI_RESULT_FILE = "psi_result.csv"

    def __init__(self, dataset: DataContext, job_id: str):

        super().__init__(dataset.ctx)
        self.job_id = job_id
        
        participant_id_list = []
        for dataset in self.dataset.datasets:
            participant_id_list.append(dataset.agency.agency_id)
        self.participant_id_list = participant_id_list

        result_list = []
        for dataset in self.dataset.datasets:
            dataset.update_path(os.path.join(self.job_id, self.PSI_RESULT_FILE))
            result_list.append(dataset)

        psi_result = DataContext(*result_list)
        return psi_result
