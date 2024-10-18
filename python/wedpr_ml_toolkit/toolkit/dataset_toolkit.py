import os
import pandas as pd
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint


class DatasetToolkit:

    def __init__(self,
                 storage_entrypoint: StorageEntryPoint,
                 storage_workspace,
                 dataset_id=None,
                 dataset_path=None,
                 agency=None,
                 values=None,
                 is_label_holder=False):
        self.dataset_id = dataset_id
        self.dataset_path = dataset_path
        self.agency = agency
        self.values = values
        self.is_label_holder = is_label_holder
        self.columns = None
        self.shape = None

        self.storage_client = storage_entrypoint
        self.storage_workspace = storage_workspace

        if self.values is not None:
            self.columns = self.values.columns
            self.shape = self.values.shape

    def load_values(self):
        # 加载hdfs的数据集
        if self.storage_client is not None:
            self.values = self.storage_client.download(self.dataset_path)
            self.columns = self.values.columns
            self.shape = self.values.shape

    def save_values(self, path=None):
        # 保存数据到hdfs目录
        if path is not None:
            self.dataset_path = path
        if not self.dataset_path.startswith(self.storage_workspace):
            self.dataset_path = os.path.join(
                self.storage_workspace, self.dataset_path)
        if self.storage_client is not None:
            self.storage_client.upload(self.values, self.dataset_path)

    def update_values(self, values: pd.DataFrame = None, path: str = None):
        # 将数据集存入hdfs相同路径，替换旧数据集
        if values is not None:
            self.values = values
            self.columns = self.values.columns
            self.shape = self.values.shape
        if path is not None:
            self.dataset_path = path
        if values is not None and self.storage_client is not None:
            self.storage_client.upload(self.values, self.dataset_path)

    def update_path(self, path: str = None):
        # 将数据集存入hdfs相同路径，替换旧数据集
        if path is not None:
            self.dataset_path = path
        if self.values is not None:
            self.values = None
