import os
import pandas as pd
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import WeDPRDatasetClient
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta


class DatasetContext:

    def __init__(self,
                 storage_entrypoint: StorageEntryPoint,
                 storage_workspace=None,
                 dataset_client: WeDPRDatasetClient = None,
                 dataset_meta=None,
                 dataset_id=None,
                 is_label_holder=False):
        self.storage_client = storage_entrypoint
        self.dataset_client = dataset_client
        self.dataset_id = dataset_id
        params = {}
        self.dataset_meta = dataset_meta
        # fetch the dataset information
        if self.dataset_meta is None and self.dataset_id is not None and self.dataset_client is not None:
            self.dataset_meta = self.dataset_client.query_dataset(
                self.dataset_id)
        self.is_label_holder = is_label_holder
        # the storage workspace
        self.storage_workspace = storage_workspace

    def load_values(self, header=None):
        # 加载hdfs的数据集
        if self.storage_client is not None:
            values = self.storage_client.download(
                self.dataset_meta.file_path, header=header)
            if values is None:
                return values, None, None
            return values, values.columns, values.shape

    def save_values(self, values: pd.DataFrame = None, path=None):
        target_path = self.dataset_meta.file_path
        # 保存数据到hdfs目录
        if path is not None:
            target_path = path
        # add the storage_workspace
        if self.storage_workspace is not None and \
                not target_path.startswith(self.storage_workspace):
            target_path = os.path.join(
                self.storage_workspace, target_path)
        if self.storage_client is not None:
            self.storage_client.upload(values, target_path)

    def update_path(self, path: str = None):
        # 将数据集存入hdfs相同路径，替换旧数据集
        if path is not None:
            self.dataset_meta.file_path = path
