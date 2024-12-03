from abc import ABC, abstractmethod
from enum import Enum
from typing import AnyStr
from ppc_common.ppc_utils import common_func
import os


class StorageType(Enum):
    HDFS = 'HDFS'


class HDFSStorageConfig:
    DEFAULT_HDFS_USER = "ppc"
    DEFAULT_HDFS_USER_PATH = "/user/"

    def __init__(self, hdfs_url: str = None,
                 hdfs_user: str = None,
                 hdfs_home: str = None,
                 enable_krb5_auth: bool = False,
                 hdfs_auth_principal: str = None,
                 hdfs_auth_secret_file_path: str = None):
        self.hdfs_url = hdfs_url
        self.hdfs_user = hdfs_user
        self.hdfs_home = hdfs_home
        self.enable_krb5_auth = enable_krb5_auth
        self.hdfs_auth_principal = hdfs_auth_principal
        self.hdfs_auth_secret_file_path = hdfs_auth_secret_file_path
        self.hdfs_krb5_ccache_path = None
        self.hdfs_host = None
        self.hdfs_port = None
        if self.hdfs_url is not None and len(self.hdfs_url) > 0:
            self.__parse__()

    def __repr__(self):
        return f"hdfs_user: {self.hdfs_user}, hdfs_home: {self.hdfs_home}, hdfs_url: {self.hdfs_url}, " \
               f"enable_krb5_auth: {self.enable_krb5_auth}, hdfs_auth_principal: {self.hdfs_auth_principal}, " \
               f"hdfs_auth_secret_file_path: {self.hdfs_auth_secret_file_path}"

    def load_config(self, config: dict, logger):
        self.hdfs_url = common_func.get_config_value(
            'HDFS_URL', None, config, True)
        self.hdfs_user = common_func.get_config_value(
            'HDFS_USER', self.DEFAULT_HDFS_USER, config, False)
        self.hdfs_home = common_func.get_config_value(
            "HDFS_HOME", os.path.join(self.DEFAULT_HDFS_USER_PATH, self.hdfs_user), config, False)

        # the auth information
        self.enable_krb5_auth = common_func.get_config_value(
            "HDFS_ENABLE_AUTH", False, config, False)
        require_auth_info = False
        if self.enable_krb5_auth is True:
            require_auth_info = True
        # the principal
        self.hdfs_auth_principal = common_func.get_config_value(
            "HDFS_AUTH_PRINCIPAL", None, config, require_auth_info
        )
        # the keytab file path
        self.hdfs_auth_secret_file_path = common_func.get_config_value(
            "HDFS_AUTH_KEYTAB_PATH", None, config, require_auth_info
        )
        self.hdfs_krb5_ccache_path = common_func.get_config_value(
            "HDFS_AUTH_CCACHE", "/tmp/ppc_model_krb5_keytab", config, False)
        if logger is not None:
            logger.info(f"*** load hdfs storage config : {self}")
        else:
            print(f"*** load hdfs storage config : {self}")
        self._check()
        # parse the host and port
        self.__parse__()

    def __parse__(self):
        hdfs_url_array = self.hdfs_url.split(":")
        if hdfs_url_array is None or len(hdfs_url_array) < 2:
            raise Exception(f"Invalid hdfs url: {self.hdfs_url}")
        host_pos = 0
        port_pos = 1
        if self.hdfs_url.startswith("hdfs"):
            host_pos = 1
            port_pos = 2
        self.hdfs_host = hdfs_url_array[host_pos]
        self.hdfs_port = int(hdfs_url_array[port_pos])

    def _check(self):
        common_func.require_non_empty("HDFS_URL", self.hdfs_url)
        common_func.require_non_empty("HDFS_USER", self.hdfs_user)
        common_func.require_non_empty("HDFS_HOME", self.hdfs_home)


class StorageApi(ABC):
    @abstractmethod
    def download_file(self, storage_path: str, local_file_path: str, enable_cache=False):
        pass

    @abstractmethod
    def upload_file(self, local_file_path: str, storage_path: str, owner=None, group=None):
        pass

    @abstractmethod
    def make_file_path(self, storage_path: str):
        pass

    @abstractmethod
    def delete_file(self, storage_path: str):
        pass

    @abstractmethod
    def save_data(self, data: AnyStr, storage_path: str):
        pass

    @abstractmethod
    def get_data(self, storage_path: str) -> AnyStr:
        pass

    @abstractmethod
    def mkdir(self, storage_path: str):
        pass

    @abstractmethod
    def file_existed(self, storage_path: str) -> bool:
        pass

    @abstractmethod
    def file_rename(self, old_storage_path: str, storage_path: str):
        pass

    @abstractmethod
    def storage_type(self):
        pass

    @abstractmethod
    def get_home_path(self):
        return ""
