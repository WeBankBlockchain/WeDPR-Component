import pandas as pd
import io

from ppc_common.deps_services import storage_loader


class HDFSApi:
    def __init__(self, hdfs_endpoint):
        self.hdfs_endpoint = hdfs_endpoint

        config_data = {}
        config_data['STORAGE_TYPE'] = 'HDFS'
        config_data['HDFS_URL'] = self.hdfs_endpoint
        config_data['HDFS_ENDPOINT'] = self.hdfs_endpoint
        self.storage_client = storage_loader.load(config_data, logger=None)

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

    def download(self, hdfs_path):
        """
        从HDFS下载数据并返回为Pandas DataFrame
        :param hdfs_path: HDFS文件路径
        :return: Pandas DataFrame
        """
        content = self.storage_client.get_data(hdfs_path)
        dataframe = pd.read_csv(io.BytesIO(content)) 
        return dataframe

    def download_byte(self, hdfs_path):
        """
        从HDFS下载数据
        :param hdfs_path: HDFS文件路径
        :return: text
        """
        content = self.storage_client.get_data(hdfs_path)
        return content
