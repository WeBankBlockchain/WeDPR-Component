import os
from typing import AnyStr

from hdfs.client import InsecureClient
from ppc_common.ppc_utils import common_func
from ppc_common.deps_services.storage_api import StorageApi, StorageType

from ppc_common.ppc_utils import utils


class HdfsStorage(StorageApi):

    DEFAULT_HDFS_USER = "ppc"
    DEFAULT_HDFS_USER_PATH = "/user/"

    # endpoint: http://127.0.0.1:50070
    def __init__(self, endpoint, hdfs_user, hdfs_home=None):
        self.endpoint = endpoint
        self._user = common_func.get_config_value(
            "HDFS_USER", HdfsStorage.DEFAULT_HDFS_USER, hdfs_user, False)
        self._hdfs_storage_path = hdfs_home
        if hdfs_home is None:
            self._hdfs_storage_path = os.path.join(
                HdfsStorage.DEFAULT_HDFS_USER_PATH, self._user)

        self.client = InsecureClient(endpoint, user=self._user)
        # print(self.client.list('/'))
        # print(self.client.list('/user/root/'))

    def get_home_path(self):
        return self._hdfs_storage_path

    def storage_type(self):
        return StorageType.HDFS

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

    def get_data(self, hdfs_path):
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
