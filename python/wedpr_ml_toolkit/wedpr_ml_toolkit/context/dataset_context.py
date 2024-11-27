import os
import pandas as pd
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import WeDPRDatasetClient
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta
import io


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

    def __repr__(self):
        return f"dataset_id: {self.dataset_id}, " \
               f"dataset_meta: {self.dataset_meta}"

    def load_values(self, header=0):
        # 加载hdfs的数据集
        if self.storage_client is not None:
            values = self.storage_client.download(
                self.dataset_meta.file_path, header=header)
            if values is None:
                return values, None, None
            return values, values.columns, values.shape
        raise Exception("Must set the storage client to load data!")

    def save_values(self, values: pd.DataFrame = None, path=None):
        # no values to save
        if values is None:
            return
        csv_buffer = io.StringIO()
        values.to_csv(csv_buffer, index=False)
        value_bytes = csv_buffer.getvalue()
        columns = values.columns.to_list()
        # update the meta firstly
        if path is None and self.dataset_meta is not None and self.dataset_meta.datasetId is not None:
            dataset_meta = DatasetMeta(dataset_id=self.dataset_meta.datasetId,
                                       dataset_fields=','.join(columns),
                                       dataset_size=len(value_bytes),
                                       dataset_record_count=len(values),
                                       dataset_column_count=len(columns))
            self.dataset_client.update_dataset(dataset_meta)
        # update the dataset meta
        if self.dataset_meta is not None:
            self.dataset_meta.datasetSize = len(value_bytes)
            self.dataset_meta.datasetFields = ','.join(columns)
            self.dataset_meta.recordCount = len(values)
            self.dataset_meta.columnCount = len(columns)
        # update the content
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
            self.storage_client.upload_bytes(value_bytes, target_path)

    def update_path(self, path: str = None):
        # 将数据集存入hdfs相同路径，替换旧数据集
        if path is not None:
            self.dataset_meta.file_path = path
