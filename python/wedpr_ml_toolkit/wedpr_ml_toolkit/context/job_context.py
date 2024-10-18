# -*- coding: utf-8 -*-
import json

from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobParam
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobInfo
from abc import abstractmethod
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobType


class JobContext:

    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, dataset: DataContext = None, my_agency=None):
        if dataset is None:
            raise Exception("Must define the job related datasets!")
        self.remote_job_client = remote_job_client
        self.project_name = project_name
        self.dataset = dataset
        self.create_agency = my_agency
        self.participant_id_list = []
        self.task_parties = []
        self.dataset_id_list = []
        self.dataset_list = []
        self.label_holder_agency = None
        self.label_columns = None
        self.__init_participant__()
        self.__init_label_information__()
        self.result_receiver_id_list = [my_agency]  # 仅限jupyter所在机构
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
            participant_id_list.append(dataset.agency.agency_id)
            dataset_id_list.append(dataset.dataset_id)
            self.task_parties.append({'userName': dataset.ctx.user_name,
                                      'agency': dataset.agency.agency_id})
        self.participant_id_list = participant_id_list
        self.dataset_id_list = dataset_id_list

    def __init_label_information__(self):
        label_holder_agency = None
        label_columns = None
        for dataset in self.dataset.datasets:
            if dataset.is_label_holder:
                label_holder_agency = dataset.agency.agency_id
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

    @abstractmethod
    def parse_result(self, result_detail):
        pass

    def fetch_job_result(self, job_id, block_until_success):
        job_result = self.query_job_status(job_id, block_until_success)
        # TODO: determine success or not here
        return self.parse_result(self.remote_job_client.query_job_detail(job_id))


class PSIJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, project_name, dataset, my_agency)
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.PSI

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_psi_format(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(), project_name=self.project_name, param=json.dumps(
            {'dataSetList': self.dataset_list}).replace('"', '\\"'))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param


class PreprocessingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, model_setting, dataset: DataContext = None, my_agency=None):
        super().__init__(remote_job_client, project_name, dataset, my_agency)
        self.model_setting = model_setting

    def get_job_type(self) -> JobType:
        return JobType.PREPROCESSING

    # TODO: build the request
    def build(self) -> JobParam:
        return None


class FeatureEngineeringJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, model_setting, dataset: DataContext = None, my_agency=None):
        super().__init__(remote_job_client, project_name, dataset, my_agency)
        self.model_setting = model_setting

    def get_job_type(self) -> JobType:
        return JobType.FEATURE_ENGINEERING

    # TODO: build the jobParam
    def build(self) -> JobParam:
        return None


class SecureLGBMTrainingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, model_setting, dataset: DataContext = None, my_agency=None):
        super().__init__(remote_job_client, project_name, dataset, my_agency)
        self.model_setting = model_setting

    def get_job_type(self) -> JobType:
        return JobType.XGB_TRAINING

    # TODO: build the jobParam
    def build(self) -> JobParam:
        return None


class SecureLGBMPredictJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, project_name: str, model_setting, dataset: DataContext = None, my_agency=None):
        super().__init__(remote_job_client, project_name, dataset, my_agency)
        self.model_setting = model_setting

    def get_job_type(self) -> JobType:
        return JobType.XGB_PREDICTING

    # TODO: build the jobParam
    def build(self) -> JobParam:
        return None
