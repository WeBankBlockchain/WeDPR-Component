import os

from wedpr_ml_toolkit.utils import utils


class DataContext:

    def __init__(self, *datasets):
        self.datasets = list(datasets)
        self.ctx = self.datasets[0].ctx

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
            if dataset.agency.agency_id in result_receiver_id_list:
                result_receiver = "true"
            else:
                result_receiver = "false"
            dataset_psi_info = {"idFields": [merge_filed],
                                "dataset": {"owner": dataset.ctx.user_name,
                                            "ownerAgency": dataset.agency.agency_id,
                                            "path": dataset.dataset_path,
                                            "storageTypeStr": "HDFS",
                                            "datasetID": dataset.dataset_id},
                                "receiveResult": result_receiver}
            dataset_psi.append(dataset_psi_info)
        return dataset_psi

    def to_model_formort(self):
        dataset_model = []
        for dataset in self.datasets:
            dataset_model.append(dataset.dataset_path)
        return dataset_model
