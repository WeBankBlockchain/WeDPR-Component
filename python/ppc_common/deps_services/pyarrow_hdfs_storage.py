# -*- coding: utf-8 -*-
import os
from ppc_common.deps_services.storage_api import StorageApi, StorageType, HDFSStorageConfig
from pyarrow.fs import HadoopFileSystem
from krbcontext.context import krbContext
from ppc_common.ppc_utils import utils
from typing import AnyStr


class PyarrowHdfsStorage(StorageApi):

    def __init__(self, hdfs_config: HDFSStorageConfig, logger, init_client: bool = True):
        if hdfs_config is None:
            raise Exception("The hdfs config must non-empty!")
        self.hdfs_config = hdfs_config
        self.logger = logger
        self._hdfs_storage_path = self.hdfs_config.hdfs_home
        self.__load_krb5_ticket__()
        self.logger.info(f"create hdfs system, config: {self.hdfs_config}")
        self.hdfs_system = HadoopFileSystem(
            host=self.hdfs_config.hdfs_host,
            port=self.hdfs_config.hdfs_port,
            kerb_ticket=self.hdfs_config.hdfs_krb5_ccache_path)
        self.logger.info(
            f"create hdfs system success, config: {self.hdfs_config}")

    def __load_krb5_ticket__(self):
        self.logger.info(
            f"load krb5 ticket into {self.hdfs_config.hdfs_krb5_ccache_path}")
        self.krb5_ctx = krbContext(
            using_keytab=True,
            principal=self.hdfs_config.hdfs_auth_principal,
            keytab_file=self.hdfs_config.hdfs_auth_secret_file_path,
            ccache_file=self.hdfs_config.hdfs_krb5_ccache_path
        )
        self.logger.info(
            f"load krb5 ticket into {self.hdfs_config.hdfs_krb5_ccache_path} success")

    def _get_hdfs_abs_path(self, hdfs_path):
        return os.path.join(self._hdfs_storage_path, hdfs_path)

    def download_file(self, hdfs_path: str, local_file_path: str, enable_cache=False):
        # hit the cache
        if enable_cache is True and utils.file_exists(local_file_path):
            return
        if utils.file_exists(local_file_path):
            utils.delete_file(local_file_path)
        local_path_dir = os.path.dirname(local_file_path)
        if len(local_path_dir) > 0 and not os.path.exists(local_path_dir):
            os.makedirs(local_path_dir)
        self.hdfs_system.copy_file(
            src=self._get_hdfs_abs_path(hdfs_path),
            dest=local_file_path)

    def upload_file(self, local_file_path: str, hdfs_path: str, owner=None, group=None):
        self.make_file_path(hdfs_path)
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        self.hdfs_system.copy_file(src=local_file_path, dest=hdfs_abs_path)
        if owner is None and group is None:
            return
        group_info = group
        if group is None:
            group_info = self.hdfs_config.hdfs_user
        owner_info = group_info
        if owner is not None:
            owner_info = owner
        self.hdfs_system.chown(
            path=hdfs_abs_path, owner=owner_info, group=group_info)

    def make_file_path(self, hdfs_path: str):
        hdfs_dir = os.path.dirname(hdfs_path)
        hdfs_abs_dir_path = self._get_hdfs_abs_path(hdfs_dir)
        if self.hdfs_system.exists(hdfs_abs_dir_path) is False:
            self.hdfs_system.mkdir(hdfs_abs_dir_path)
        return

    def delete_file(self, hdfs_path: str):
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        self.hdfs_system.rm(path=hdfs_abs_path, recursive=True)

    def save_data(self, data: AnyStr, hdfs_path: str):
        self.make_file_path(hdfs_path)
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        with self.hdfs_system.open(path=hdfs_abs_path, mode="w", buffer_size=4096) as hdfs_fp:
            hdfs_fp.write(data)

    def get_data(self, hdfs_path: str) -> AnyStr:
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        with self.hdfs_system.open(path=hdfs_abs_path, mode="rb", buffer_size=4096) as hdfs_fp:
            return hdfs_fp.read()

    def mkdir(self, hdfs_path: str):
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        self.hdfs_system.mkdir(hdfs_abs_path)

    def file_existed(self, hdfs_path: str) -> bool:
        hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        return self.hdfs_system.exists(hdfs_abs_path)

    def file_rename(self, old_hdfs_path, hdfs_path):
        old_hdfs_abs_path = self._get_hdfs_abs_path(old_hdfs_path)
        new_hdfs_abs_path = self._get_hdfs_abs_path(hdfs_path)
        self.hdfs_system.rename(path=old_hdfs_abs_path,
                                new_path=new_hdfs_abs_path)

    def storage_type(self):
        return StorageType.HDFS

    def get_home_path(self):
        return self._hdfs_storage_path
