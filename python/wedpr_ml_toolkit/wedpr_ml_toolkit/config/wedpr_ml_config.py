# -*- coding: utf-8 -*-
import os
from wedpr_ml_toolkit.common.utils.base_object import BaseObject
from wedpr_ml_toolkit.common.utils.constant import Constant
from wedpr_ml_toolkit.common.utils.properies_parser import Properties
from wedpr_ml_toolkit.common.utils import utils


class AuthConfig(BaseObject):
    def __init__(self, access_key_id: str = None, access_key_secret: str = None, remote_entrypoints: str = None, nonce_len: int = 5):
        self.access_key_id = access_key_id
        self.access_key_secret = access_key_secret
        self.remote_entrypoints = remote_entrypoints
        self.nonce_len = nonce_len

    def get_remote_entrypoints_list(self) -> []:
        if self.remote_entrypoints is None:
            return None
        return self.remote_entrypoints.split(',')


class JobConfig(BaseObject):
    def __init__(self, polling_interval_s: int = 5, max_retries: int = 2, retry_delay_s: int = 5,
                 submit_job_uri: str = Constant.DEFAULT_SUBMIT_JOB_URI,
                 query_job_status_uri: str = Constant.DEFAULT_QUERY_JOB_STATUS_URL,
                 query_job_detail_uri: str = Constant.DEFAULT_QUERY_JOB_DETAIL_URL):
        self.polling_interval_s = polling_interval_s
        self.max_retries = max_retries
        self.retry_delay_s = retry_delay_s
        self.submit_job_uri = submit_job_uri
        self.query_job_status_uri = query_job_status_uri
        self.query_job_detail_uri = query_job_detail_uri


class DatasetConfig(BaseObject):
    def __init__(self,
                 query_dataset_uri=Constant.DEFAULT_QUERY_DATASET_URL,
                 update_dataset_uri=Constant.DEFAULT_UPDATED_DATASET_URL):
        self.query_dataset_uri = query_dataset_uri
        self.update_dataset_uri = update_dataset_uri


class UserConfig(BaseObject):
    def __init__(self, agency_name: str = None, workspace_path: str = None, user_name: str = None):
        self.agency_name = agency_name
        self.workspace_path = workspace_path
        self.user = user_name

    def get_workspace_path(self):
        return os.path.join(self.workspace_path, self.user)


class StorageConfig(BaseObject):
    def __init__(self,
                 user_config: UserConfig,
                 storage_endpoint: str = None,
                 enable_krb5_auth: str = "False",
                 hdfs_auth_principal: str = None,
                 hdfs_auth_password: str = None,
                 hdfs_hostname_override: str = None):
        self.user_config = user_config
        self.storage_endpoint = storage_endpoint
        self.enable_krb5_auth = enable_krb5_auth
        self.hdfs_auth_principal = hdfs_auth_principal
        self.hdfs_auth_password = hdfs_auth_password
        self.hdfs_hostname_override = hdfs_hostname_override

    def get_enable_krb5_auth(self) -> bool:
        if self.enable_krb5_auth is None or len(self.enable_krb5_auth) == 0:
            return False
        return utils.str_to_bool(self.enable_krb5_auth)

    def __repr__(self):
        return f"hdfs_user: {self.hdfs_user}, hdfs_home: {self.hdfs_home}, hdfs_url: {self.hdfs_url}, " \
            f"enable_krb5_auth: {self.enable_krb5_auth}, hdfs_auth_principal: {self.hdfs_auth_principal}"


class HttpConfig(BaseObject):
    def __init__(self, timeout_seconds=3):
        self.timeout_seconds = timeout_seconds


class WeDPRMlConfig:
    def __init__(self, config_dict):
        self.auth_config = AuthConfig()
        self.auth_config.set_params(**config_dict)
        self.job_config = JobConfig()
        self.job_config.set_params(**config_dict)
        self.user_config = UserConfig()
        self.user_config.set_params(**config_dict)
        self.http_config = HttpConfig()
        self.http_config.set_params(**config_dict)
        self.dataset_config = DatasetConfig()
        self.storage_config = StorageConfig(self.user_config)
        self.storage_config.set_params(**config_dict)


class WeDPRMlConfigBuilder:
    @staticmethod
    def build(config_dict) -> WeDPRMlConfig:
        return WeDPRMlConfig(config_dict)

    @staticmethod
    def build_from_properties_file(config_file_path):
        if not os.path.exists(config_file_path):
            raise Exception(
                f"build WeDPRMlConfig failed for the config file {config_file_path} not exits!")
        properties = Properties(config_file_path)
        return WeDPRMlConfigBuilder.build(properties.getProperties())
