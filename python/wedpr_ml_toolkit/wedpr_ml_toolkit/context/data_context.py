import os

from wedpr_ml_toolkit.common import utils
from wedpr_ml_toolkit.context.dataset_context import DatasetContext


class DataContext:

    def __init__(self, *datasets):
        self.datasets = list(datasets)

    def to_psi_format(self, merge_filed, result_receiver_id_list):
        dataset_psi = []
        for dataset in self.datasets:
            if dataset.dataset_meta.ownerAgencyName in result_receiver_id_list:
                result_receiver = True
            else:
                result_receiver = False
            dataset_psi_info = self.__generate_dataset_info__(
                merge_filed, result_receiver, None, dataset)
            dataset_psi.append(dataset_psi_info)
        return dataset_psi

    def __generate_dataset_info__(self, id_field: str, receive_result: bool, label_provider: bool, dataset: DatasetContext):
        return {"idFields": [id_field],
                "dataset": {"ownerAgency": dataset.dataset_meta.ownerAgencyName,
                            "datasetID": dataset.dataset_id},
                "receiveResult": receive_result,
                "labelProvider": label_provider}

    def to_model_formort(self, merge_filed, result_receiver_id_list):
        dataset_model = []
        for dataset in self.datasets:
            if dataset.dataset_meta.ownerAgencyName in result_receiver_id_list:
                result_receiver = True
            else:
                result_receiver = False
            if dataset.is_label_holder:
                label_provider = True
            else:
                label_provider = False
            dataset_psi_info = self.__generate_dataset_info__(
                merge_filed, result_receiver, label_provider, dataset)
            dataset_model.append(dataset_psi_info)
        return dataset_model
