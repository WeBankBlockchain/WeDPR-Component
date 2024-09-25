import os

from ppc_dev.utils import utils


class DataContext:

    def __init__(self, *datasets):
        self.datasets = list(datasets)
        self.ctx = self.datasets[0].ctx
        
        self._check_datasets()

    def _save_dataset(self, dataset):
        if dataset.dataset_path is None:
            dataset.dataset_id = utils.make_id(utils.IdPrefixEnum.DATASET.value)
            dataset.dataset_path = os.path.join(dataset.ctx.workspace, dataset.dataset_id)
            if self.storage_client is not None:
                self.storage_client.upload(self.values, self.dataset_path)

    def _check_datasets(self):
        for dataset in self.datasets:
            self._save_dataset(dataset)

    def to_psi_format(self):
        dataset_psi = []
        for dataset in self.datasets:
            dataset_psi.append(dataset.dataset_path)
        return dataset_psi

    def to_model_formort(self):
        dataset_model = []
        for dataset in self.datasets:
            dataset_model.append(dataset.dataset_path)
        return dataset_model
