import os
from typing import AnyStr

from hdfs.client import InsecureClient
from ppc_common.ppc_utils import common_func
from ppc_common.deps_services.storage_api import StorageApi, StorageType, HDFSStorageConfig

from ppc_common.ppc_utils import utils


class HdfsStorage(StorageApi):

    # endpoint: http://127.0.0.1:50070
    def __init__(self, hdfs_config: HDFSStorageConfig, init_client: bool = True):
        self.endpoint = hdfs_config.hdfs_url
        self._user = hdfs_config.hdfs_user
        self._hdfs_storage_path = hdfs_config.hdfs_home
        if init_client:
            self.client = InsecureClient(self.endpoint, user=self._user)

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

    def upload_file(self, local_file_path, hdfs_path, owner=None, group=None):
        self.make_file_path(hdfs_path)
        hdfs_abs_path = os.path.join(self._hdfs_storage_path, hdfs_path)
        self.client.upload(hdfs_abs_path,
                           local_file_path, overwrite=True)
        if owner is None and group is None:
            return
        group_info = group
        if group is None:
            group_info = self._user
        owner_info = group_info
        if owner is not None:
            owner_info = owner
        # set the permission information
        self.client.set_owner(hdfs_path=hdfs_abs_path,
                              owner=owner_info, group=group_info)
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
