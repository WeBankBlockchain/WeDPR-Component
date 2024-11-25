import os

from wedpr_ml_toolkit.common import utils
from wedpr_ml_toolkit.context.dataset_context import DatasetContext


class DataContext:

    def __init__(self, *datasets):
        self.datasets = list(datasets)

        self._check_datasets()

    def _save_dataset(self, dataset: DatasetContext):
        file_path = dataset.dataset_meta.file_path
        if file_path is None:
            dataset.dataset_id = utils.make_id(
                utils.IdPrefixEnum.DATASET.value)
            file_path = os.path.join(
                dataset.storage_workspace, dataset.dataset_id)
            if dataset.storage_client is not None:
                dataset.storage_client.upload(
                    dataset.values, file_path)

    def _check_datasets(self):
        for dataset in self.datasets:
            self._save_dataset(dataset)

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
                "dataset": {"owner": dataset.dataset_meta.ownerUserName,
                            "ownerAgency": dataset.dataset_meta.ownerAgencyName,
                            "path": dataset.dataset_meta.file_path,
                            "storageTypeStr": "HDFS",
                            "datasetID": dataset.dataset_id},
                "receiveResult": receive_result,
                "labelProvider": label_provider
                }

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
