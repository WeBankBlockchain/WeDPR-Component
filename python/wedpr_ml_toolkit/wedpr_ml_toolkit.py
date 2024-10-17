# -*- coding: utf-8 -*-

from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfig
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.context.job_context import PSIJobContext
from wedpr_ml_toolkit.context.job_context import PreprocessingJobContext
from wedpr_ml_toolkit.context.job_context import FeatureEngineeringJobContext
from wedpr_ml_toolkit.context.job_context import SecureLGBMPredictJobContext
from wedpr_ml_toolkit.context.job_context import SecureLGBMTrainingJobContext
from wedpr_ml_toolkit.context.data_context import DataContext


class WeDPRMlToolkit:
    def __init__(self, config: WeDPRMlConfig):
        self.config = config
        self.remote_job_client = WeDPRRemoteJobClient(
            self.config.auth_config, self.config.job_config)
        self.storage_entry_point = StorageEntryPoint(self.config.user_config,
                                                     self.config.storage_config)

    def get_config(self) -> WeDPRMlConfig:
        return self.config

    def get_remote_job_client(self) -> WeDPRRemoteJobClient:
        return self.remote_job_client

    def get_storage_entry_point(self) -> StorageEntryPoint:
        return self.storage_entry_point

    def submit(self, job_param):
        return self.remote_job_client.submit_job(job_param)

    def query_job_status(self, job_id, block_until_success=False):
        if not block_until_success:
            return self.remote_job_client.query_job_result(job_id)
        return self.remote_job_client.wait_job_finished()

    def query_job_detail(self, job_id, block_until_success):
        job_result = self.query_job_status(job_id, block_until_success)
        # TODO: determine success or not here
        return self.remote_job_client.query_job_detail(job_id)

    def build_job_context(self, job_type: JobType, project_name: str, dataset: DataContext, model_setting=None, id_fields='id'):
        if job_type == JobType.PSI:
            return PSIJobContext(self.remote_job_client, project_name, dataset, self.config.agency_config.agency_name, id_fields)
        if job_type == JobType.PREPROCESSING:
            return PreprocessingJobContext(self.remote_job_client, project_name, model_setting, dataset, self.config.agency_config.agency_name)
        if job_type == JobType.FEATURE_ENGINEERING:
            return FeatureEngineeringJobContext(self.remote_job_client, project_name, model_setting, dataset, self.config.agency_config.agency_name)
        if job_type == JobType.XGB_TRAINING:
            return SecureLGBMTrainingJobContext(self.remote_job_client, project_name, model_setting, dataset, self.config.agency_config.agency_name)
        if job_type == JobType.XGB_PREDICTING:
            return SecureLGBMPredictJobContext(self.remote_job_client, project_name, model_setting, dataset, self.config.agency_config.agency_name)
