import os
from typing import AnyStr

from hdfs.client import InsecureClient
from wedpr_ml_toolkit.common.utils import utils
from wedpr_ml_toolkit.config.wedpr_ml_config import StorageConfig
from hdfs.ext.kerberos import KerberosClient


class HdfsStorageImpl:
    # endpoint: http://127.0.0.1:50070
    def __init__(self, storage_config: StorageConfig):
        self.hdfs_config = storage_config
        self._hdfs_storage_path = self.hdfs_config.user_config.get_workspace_path()
        self.client = None
        if self.hdfs_config.get_enable_krb5_auth() is True:
            self.client = KerberosClient(
                url=self.hdfs_config.storage_endpoint,
                principal=self.hdfs_config.hdfs_auth_principal,
                hostname_override=self.hdfs_config.hdfs_hostname_override,
                password=self.hdfs_config.hdfs_auth_password,
                timeout=10000)
        else:
            self.client = InsecureClient(
                self.hdfs_config.storage_endpoint,
                user=self.hdfs_config.user_config.user)

    def get_home_path(self):
        return self._hdfs_storage_path

    def download_file(self, hdfs_path, local_file_path, enable_cache=False):
        # hit the cache
        if enable_cache is True and utils.file_exists(local_file_path):
            return
        if utils.file_exists(local_file_path):
            utils.delete_file(local_file_path)
        local_path = os.path.dirname(local_file_path)
        if len(local_path) > 0 and not os.path.exists(local_path):
            os.makedirs(local_path)
        self.client.download(os.path.join(self._hdfs_storage_path,
                             hdfs_path), local_file_path)
        return

    def upload_file(self, local_file_path, hdfs_path):
        self.make_file_path(hdfs_path)
        self.client.upload(os.path.join(self._hdfs_storage_path, hdfs_path),
                           local_file_path, overwrite=True)
        return

    def make_file_path(self, hdfs_path):
        hdfs_dir = os.path.dirname(hdfs_path)
        if self.client.status(os.path.join(self._hdfs_storage_path, hdfs_dir), strict=False) is None:
            self.client.makedirs(os.path.join(
                self._hdfs_storage_path, hdfs_dir))
        return

    def delete_file(self, hdfs_path):
        self.client.delete(os.path.join(
            self._hdfs_storage_path, hdfs_path), recursive=True)
        return

    def save_data(self, data: AnyStr, hdfs_path):
        self.make_file_path(hdfs_path)
        self.client.write(os.path.join(self._hdfs_storage_path,
                          hdfs_path), data, overwrite=True)
        return

    def get_data(self, hdfs_path) -> AnyStr:
        with self.client.read(os.path.join(self._hdfs_storage_path, hdfs_path)) as reader:
            content = reader.read()
        return content

    def mkdir(self, hdfs_dir):
        self.client.makedirs(hdfs_dir)

    def file_existed(self, hdfs_path):
        if self.client.status(os.path.join(self._hdfs_storage_path, hdfs_path), strict=False) is None:
            return False
        return True

    def file_rename(self, old_hdfs_path, hdfs_path):
        old_path = os.path.join(self._hdfs_storage_path, old_hdfs_path)
        new_path = os.path.join(self._hdfs_storage_path, hdfs_path)
        # return for the file not exists
        if not self.file_existed(old_path):
            return
        parent_path = os.path.dirname(new_path)
        if len(parent_path) > 0 and not self.file_existed(parent_path):
            self.mkdir(parent_path)
        self.client.rename(old_path, new_path)
        return
