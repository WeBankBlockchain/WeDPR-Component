import os

from wedpr_ml_toolkit.common import utils


class DataContext:

    def __init__(self, *datasets):
        self.datasets = list(datasets)

        self._check_datasets()

    def _save_dataset(self, dataset):
        if dataset.dataset_path is None:
            dataset.dataset_id = utils.make_id(
                utils.IdPrefixEnum.DATASET.value)
            dataset.dataset_path = os.path.join(
                dataset.storage_workspace, dataset.dataset_id)
            if dataset.storage_client is not None:
                dataset.storage_client.upload(
                    dataset.values, dataset.dataset_path)

    def _check_datasets(self):
        for dataset in self.datasets:
            self._save_dataset(dataset)

    def to_psi_format(self, merge_filed, result_receiver_id_list):
        dataset_psi = []
        for dataset in self.datasets:
            if dataset.agency in result_receiver_id_list:
                result_receiver = True
            else:
                result_receiver = False
            dataset_psi_info = {"idFields": [merge_filed],
                                "dataset": {"owner": dataset.dataset_owner,
                                            "ownerAgency": dataset.agency,
                                            "path": dataset.dataset_path,
                                            "storageTypeStr": "HDFS",
                                            "datasetID": dataset.dataset_id},
                                "receiveResult": result_receiver}
            dataset_psi.append(dataset_psi_info)
        return dataset_psi

    def to_model_formort(self, merge_filed, result_receiver_id_list):
        dataset_model = []
        for dataset in self.datasets:
            if dataset.agency in result_receiver_id_list:
                result_receiver = True
            else:
                result_receiver = False
            if dataset.is_label_holder:
                label_provider = True
            else:
                label_provider = False
            dataset_psi_info = {"idFields": [merge_filed],
                                "dataset": {"owner": dataset.dataset_owner,
                                            "ownerAgency": dataset.agency,
                                            "path": dataset.dataset_path,
                                            "storageTypeStr": "HDFS",
                                            "datasetID": dataset.dataset_id},
                                "labelProvider": label_provider,
                                "receiveResult": result_receiver}
            dataset_model.append(dataset_psi_info)
        return dataset_model
