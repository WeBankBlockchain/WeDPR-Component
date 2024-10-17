# -*- coding: utf-8 -*-

from config.wedpr_ml_config import WeDPRMlConfig
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint


class WeDPRMlToolkit:
    def __init__(self, config: WeDPRMlConfig):
        self.config = config
        self.remote_job_client = WeDPRRemoteJobClient(
            self.config.auth_config, self.config.job_config)
        self.storage_entry_point = StorageEntryPoint(
            self.config.storage_config)

    def get_config(self) -> WeDPRMlConfig:
        return self.config

    def get_remote_job_client(self) -> WeDPRRemoteJobClient:
        return self.remote_job_client

    def get_storage_entry_point(self) -> StorageEntryPoint:
        return self.storage_entry_point

    def submit(self, job_param):
        return self.remote_job_client.submit_job(job_param)
