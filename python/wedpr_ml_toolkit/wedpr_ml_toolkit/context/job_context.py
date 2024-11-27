# -*- coding: utf-8 -*-
import json

from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobParam
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobInfo
from abc import abstractmethod
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobType, ModelType
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta
from wedpr_ml_toolkit.context.model_setting import ModelSetting
from wedpr_ml_toolkit.config.wedpr_ml_config import UserConfig


class JobContext:

    def __init__(self, remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 dataset: DataContext,
                 user_config: UserConfig):
        if dataset is None:
            raise Exception("Must define the job related datasets!")
        self.dataset = dataset
        self.user_config = user_config

        self.remote_job_client = remote_job_client
        self.storage_entry_point = storage_entry_point
        self.project_id = project_id
        self.participant_id_list = []
        self.task_parties = []
        self.dataset_id_list = []
        self.dataset_list = []
        self.label_holder_agency = None
        self.label_columns = None
        self.__init_participant__()
        self.__init_label_information__()
        self.result_receiver_id_list = [
            self.user_config.agency_name]  # 仅限jupyter所在机构
        self.__check__()

    def __check__(self):
        """
        校验机构数和任务是否匹配
        """
        if len(self.participant_id_list) < 2:
            raise Exception("至少需要传入两个机构")
        if not self.label_holder_agency or self.label_holder_agency not in self.participant_id_list:
            raise Exception("数据集中标签提供方配置错误")

    def __init_participant__(self):
        participant_id_list = []
        dataset_id_list = []
        for dataset in self.dataset.datasets:
            participant_id_list.append(dataset.dataset_meta.ownerAgencyName)
            dataset_id_list.append(dataset.dataset_id)
            self.task_parties.append({'userName': dataset.dataset_meta.ownerUserName,
                                      'agency': dataset.dataset_meta.ownerAgencyName})
        self.participant_id_list = participant_id_list
        self.dataset_id_list = dataset_id_list

    def __init_label_information__(self):
        label_holder_agency = None
        label_columns = None
        for dataset in self.dataset.datasets:
            if dataset.is_label_holder:
                label_holder_agency = dataset.dataset_meta.ownerAgencyName
                label_columns = 'y'
        self.label_holder_agency = label_holder_agency
        self.label_columns = label_columns

    @abstractmethod
    def build(self) -> JobParam:
        pass

    @abstractmethod
    def get_job_type(self) -> JobType:
        pass

    def submit(self):
        return self.remote_job_client.submit_job(self.build())

    def fetch_job_result(self, job_id, block_until_success):
        # query_job_detail
        result_detail = self.remote_job_client.query_job_detail(
            job_id, block_until_success)
        return result_detail


class PSIJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 dataset: DataContext,
                 user_config: UserConfig,
                 merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.PSI

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_psi_format(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps({'dataSetList': self.dataset_list}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class PreprocessingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str, model_setting: ModelSetting,
                 dataset: DataContext, user_config: UserConfig,
                 merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.PREPROCESSING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps({'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict()}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class FeatureEngineeringJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 model_setting: ModelSetting,
                 dataset: DataContext,
                 user_config: UserConfig, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.FEATURE_ENGINEERING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps({'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict()}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class SecureLGBMTrainingJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 model_setting: ModelSetting,
                 dataset: DataContext,
                 user_config: UserConfig,
                 merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.XGB_TRAINING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps({'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict()}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class SecureLGBMPredictJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 model_setting: ModelSetting, predict_algorithm,
                 dataset: DataContext,
                 user_config: UserConfig,
                 merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field
        self.predict_algorithm = predict_algorithm

    def get_job_type(self) -> JobType:
        return JobType.XGB_PREDICTING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict(),
             'modelPredictAlgorithm': json.dumps(self.predict_algorithm)}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class SecureLRTrainingJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 model_setting:  ModelSetting,
                 dataset: DataContext,
                 user_config: UserConfig, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.LR_TRAINING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict()}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class SecureLRPredictJobContext(JobContext):
    def __init__(self,
                 remote_job_client: WeDPRRemoteJobClient,
                 storage_entry_point: StorageEntryPoint,
                 project_id: str,
                 model_setting: ModelSetting, predict_algorithm,
                 dataset: DataContext,
                 user_config: UserConfig,
                 merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point,
                         project_id, dataset, user_config)
        self.model_setting = model_setting
        self.merge_field = merge_field
        self.predict_algorithm = predict_algorithm

    def get_job_type(self) -> JobType:
        return JobType.LR_PREDICTING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(),
                           project_id=self.project_id,
                           param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting.as_dict(),
             'modelPredictAlgorithm': json.dumps(self.predict_algorithm)}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param
