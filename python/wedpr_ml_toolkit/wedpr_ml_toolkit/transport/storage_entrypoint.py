import pandas as pd
import io

from wedpr_ml_toolkit.common.utils.hdfs_storage_impl import HdfsStorageImpl
from wedpr_ml_toolkit.config.wedpr_ml_config import StorageConfig
from wedpr_ml_toolkit.config.wedpr_ml_config import UserConfig


class StorageEntryPoint:
    def __init__(self, user_config: UserConfig, storage_config: StorageConfig):
        self.storage_config = storage_config
        self.user_config = user_config
        self.storage_client = HdfsStorageImpl(
            self.storage_config.storage_endpoint, self.user_config.user, self.user_config.get_workspace_path())

    def upload(self, dataframe, hdfs_path):
        """
        上传Pandas DataFrame到HDFS
        :param dataframe: 要上传的Pandas DataFrame
        :param hdfs_path: HDFS目标路径
        :return: 响应信息
        """
        # 将DataFrame转换为CSV格式
        csv_buffer = io.StringIO()
        dataframe.to_csv(csv_buffer, index=False)
        self.storage_client.save_data(csv_buffer.getvalue(), hdfs_path)
        return

    def download(self, hdfs_path, header=None):
        """
        从HDFS下载数据并返回为Pandas DataFrame
        :param hdfs_path: HDFS文件路径
        :return: Pandas DataFrame
        """
        content = self.storage_client.get_data(hdfs_path)
        dataframe = pd.read_csv(io.BytesIO(content), header=header)
        return dataframe

    def download_byte(self, hdfs_path):
        """
        从HDFS下载数据
        :param hdfs_path: HDFS文件路径
        :return: text
        """
        content = self.storage_client.get_data(hdfs_path)
        return content
