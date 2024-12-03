# -*- coding: utf-8 -*-

from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfig
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobInfo
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import WeDPRDatasetClient
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.context.job_context import PSIJobContext
from wedpr_ml_toolkit.context.job_context import PreprocessingJobContext
from wedpr_ml_toolkit.context.job_context import FeatureEngineeringJobContext
from wedpr_ml_toolkit.context.job_context import SecureLGBMPredictJobContext
from wedpr_ml_toolkit.context.job_context import SecureLGBMTrainingJobContext
from wedpr_ml_toolkit.context.job_context import SecureLRPredictJobContext
from wedpr_ml_toolkit.context.job_context import SecureLRTrainingJobContext
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.context.model_setting import ModelSetting
from wedpr_ml_toolkit.context.result.result_context import ResultContext
from wedpr_ml_toolkit.context.result.psi_result_context import PSIResultContext
from wedpr_ml_toolkit.context.result.fe_result_context import FeResultContext
from wedpr_ml_toolkit.context.result.fe_result_context import PreprocessingResultContext
from wedpr_ml_toolkit.context.result.model_result_context import TrainResultContext
from wedpr_ml_toolkit.context.result.model_result_context import PredictResultContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobDetailResponse


class WeDPRMlToolkit:
    def __init__(self, config: WeDPRMlConfig):
        self.config = config
        self.remote_job_client = WeDPRRemoteJobClient(
            self.config.http_config, self.config.auth_config, self.config.job_config)
        self.dataset_client = WeDPRDatasetClient(http_config=self.config.http_config,
                                                 auth_config=self.config.auth_config,
                                                 dataset_config=self.config.dataset_config)
        self.storage_entry_point = StorageEntryPoint(
            self.config.storage_config)

    def get_config(self) -> WeDPRMlConfig:
        return self.config

    def get_dataset_client(self) -> WeDPRDatasetClient:
        return self.dataset_client

    def get_remote_job_client(self) -> WeDPRRemoteJobClient:
        return self.remote_job_client

    def get_storage_entry_point(self) -> StorageEntryPoint:
        return self.storage_entry_point

    def submit(self, job_param):
        return self.remote_job_client.submit_job(job_param)

    def query_job_status(self, job_id, block_until_finish=False) -> JobInfo:
        return self.remote_job_client.poll_job_result(job_id, block_until_finish)

    def query_job_detail(self, job_id, block_until_finish=False) -> JobDetailResponse:
        return self.remote_job_client.query_job_detail(job_id, block_until_finish)

    def build_result_context(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        job_type = ResultContext.check_and_get_job_type(
            job_context, job_result_detail)
        if job_type == JobType.PSI.name:
            return PSIResultContext(job_context, job_result_detail)
        if job_type == JobType.PREPROCESSING.name:
            return PreprocessingResultContext(job_context, job_result_detail)
        if job_type == JobType.FEATURE_ENGINEERING.name:
            return FeResultContext(job_context, job_result_detail)
        if job_type == JobType.XGB_TRAINING.name or job_type == JobType.LR_TRAINING.name:
            return TrainResultContext(job_context, job_result_detail)
        if job_type == JobType.XGB_PREDICTING.name or job_type == JobType.LR_PREDICTING.name:
            return PredictResultContext(job_context, job_result_detail)
        raise Exception(f"Unsupported job type: {job_type}")

    def build_job_context(self, job_type: JobType, project_id: str, dataset: DataContext, model_setting: ModelSetting = None,
                          id_fields='id', predict_algorithm=None):
        if job_type == JobType.PSI:
            return PSIJobContext(self.remote_job_client, self.storage_entry_point, project_id, dataset,
                                 self.config.user_config, id_fields)
        if job_type == JobType.PREPROCESSING:
            return PreprocessingJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                           model_setting, dataset, self.config.user_config, id_fields)
        if job_type == JobType.FEATURE_ENGINEERING:
            return FeatureEngineeringJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                                model_setting, dataset, self.config.user_config, id_fields)
        if job_type == JobType.XGB_TRAINING:
            return SecureLGBMTrainingJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                                model_setting, dataset, self.config.user_config, id_fields)
        if job_type == JobType.XGB_PREDICTING:
            return SecureLGBMPredictJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                               model_setting, predict_algorithm, dataset, self.config.user_config, id_fields)
        if job_type == JobType.LR_TRAINING:
            return SecureLRTrainingJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                              model_setting, dataset, self.config.user_config, id_fields)
        if job_type == JobType.LR_PREDICTING:
            return SecureLRPredictJobContext(self.remote_job_client, self.storage_entry_point, project_id,
                                             model_setting, predict_algorithm, dataset, self.config.user_config, id_fields)
        raise Exception(f"Unsupported job type: {job_type}")
