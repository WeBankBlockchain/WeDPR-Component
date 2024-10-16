import os
import pandas as pd

from wedpr_ml_toolkit.common.base_context import BaseContext
from wedpr_ml_toolkit.job_exceuter.hdfs_client import HDFSApi


class WedprData:

    def __init__(self, 
                 ctx: BaseContext, 
                 dataset_id=None, 
                 dataset_path=None, 
                 agency=None, 
                 values=None, 
                 is_label_holder=False):
    
        # super().__init__(project_id)
        self.ctx = ctx

        self.dataset_id = dataset_id
        self.dataset_path = dataset_path
        self.agency = agency
        self.values = values
        self.is_label_holder = is_label_holder
        self.columns = None
        self.shape = None

        self.storage_client = HDFSApi(self.ctx.hdfs_endpoint)
        self.storage_workspace = os.path.join(self.ctx.workspace, self.agency.agency_id, self.ctx.user_name, 'share')

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
        if not self.dataset_path.startswith(self.ctx.workspace):
            self.dataset_path = os.path.join(self.storage_workspace, self.dataset_path)
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
