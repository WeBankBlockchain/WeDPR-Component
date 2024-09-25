import requests
import pandas as pd
import io


class HDFSApi:
    def __init__(self, base_url):
        self.base_url = base_url

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
        
        # 发送PUT请求上传CSV数据
        response = requests.put(
            f"{self.base_url}/upload?path={hdfs_path}",
            data=csv_buffer.getvalue(),
            headers={'Content-Type': 'text/csv'}
        )
        return response.json()

    def download(self, hdfs_path):
        """
        从HDFS下载数据并返回为Pandas DataFrame
        :param hdfs_path: HDFS文件路径
        :return: Pandas DataFrame
        """
        response = requests.get(f"{self.base_url}/download?path={hdfs_path}")
        if response.status_code == 200:
            # 读取CSV数据并转换为DataFrame
            dataframe = pd.read_csv(io.StringIO(response.text))
            return dataframe
        else:
            raise Exception(f"下载失败: {response.json()}")

    def download_data(self, hdfs_path):
        """
        从HDFS下载数据并返回为Pandas DataFrame
        :param hdfs_path: HDFS文件路径
        :return: text
        """
        response = requests.get(f"{self.base_url}/download?path={hdfs_path}")
        if response.status_code == 200:
            return response.text
        else:
            raise Exception(f"下载失败: {response.json()}")
