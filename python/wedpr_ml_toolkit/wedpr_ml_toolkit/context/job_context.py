# -*- coding: utf-8 -*-
import json

from wedpr_ml_toolkit.toolkit.dataset_toolkit import DatasetToolkit
from wedpr_ml_toolkit.transport.storage_entrypoint import StorageEntryPoint
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobParam
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobInfo
from abc import abstractmethod
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import WeDPRRemoteJobClient
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobType, ModelType
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import ModelResult


class JobContext:

    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, dataset: DataContext = None, my_agency=None):
        if dataset is None:
            raise Exception("Must define the job related datasets!")
        self.remote_job_client = remote_job_client
        self.storage_entry_point = storage_entry_point
        self.project_id = project_id
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
            participant_id_list.append(dataset.agency)
            dataset_id_list.append(dataset.dataset_id)
            self.task_parties.append({'userName': dataset.dataset_owner,
                                      'agency': dataset.agency})
        self.participant_id_list = participant_id_list
        self.dataset_id_list = dataset_id_list

    def __init_label_information__(self):
        label_holder_agency = None
        label_columns = None
        for dataset in self.dataset.datasets:
            if dataset.is_label_holder:
                label_holder_agency = dataset.agency
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
    def parse_result(self, job_id, block_until_success):
        pass

    def fetch_job_result(self, job_id, block_until_success):
        # job_result = self.query_job_status(job_id, block_until_success)
        # # TODO: determine success or not here
        # return self.parse_result(self.remote_job_client.query_job_detail(job_id, block_until_success))

        # # query_job_status
        # job_result = self.remote_job_client.poll_job_result(job_id, block_until_success)
        # # failed case
        # if job_result == None or job_result.job_status == None or (not job_result.job_status.run_success()):
        #     raise Exception(f'job {job_id} running failed!')
        # # success case
        # ...

        # query_job_detail
        result_detail = self.remote_job_client.query_job_detail(job_id, block_until_success)
        return result_detail


class PSIJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.PSI

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_psi_format(
            self.merge_field, self.result_receiver_id_list)
        # job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
        #     {'dataSetList': self.dataset_list}).replace('"', '\\"'))
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)

        psi_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                storage_workspace=None, 
                                dataset_owner=self.storage_entry_point.user_config.user,
                                dataset_path=result_detail.resultFileInfo['path'], agency=self.create_agency)

        return psi_result


class PreprocessingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.PREPROCESSING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)

        pre_result = result_detail
        # pre_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
        #                         storage_workspace=None, 
        #                         dataset_owner=self.storage_entry_point.user_config.user,
        #                         dataset_path=result_detail.resultFileInfo['path'], agency=self.create_agency)

        return pre_result


class FeatureEngineeringJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.FEATURE_ENGINEERING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)

        fe_result = result_detail
        # fe_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
        #                         storage_workspace=None, 
        #                         dataset_owner=self.storage_entry_point.user_config.user,
        #                         dataset_path=result_detail.resultFileInfo['path'], agency=self.create_agency)

        return fe_result


class SecureLGBMTrainingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.XGB_TRAINING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        # job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
        #     {'dataSetList': self.dataset_list}).replace('"', '\\"'))
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)
        # result_detail.modelResultDetail['ModelResult']
        train_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['trainResultPath'], agency=self.create_agency)
        test_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['testResultPath'], agency=self.create_agency)

        xgb_result = ModelResult(job_id, train_result, test_result, result_detail.model, ModelType.XGB_MODEL_SETTING.name)
        return xgb_result


class SecureLGBMPredictJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, predict_algorithm, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field
        self.predict_algorithm = predict_algorithm

    def get_job_type(self) -> JobType:
        return JobType.XGB_PREDICTING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        # job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
        #     {'dataSetList': self.dataset_list}).replace('"', '\\"'))
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting, 'modelPredictAlgorithm': json.dumps(self.predict_algorithm)}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)
        test_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['testResultPath'], agency=self.create_agency)

        xgb_result = ModelResult(job_id, test_result=test_result)
        return xgb_result


class SecureLRTrainingJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field

    def get_job_type(self) -> JobType:
        return JobType.LR_TRAINING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)
        # result_detail.modelResultDetail['ModelResult']
        train_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['trainResultPath'], agency=self.create_agency)
        test_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['testResultPath'], agency=self.create_agency)

        lr_result = ModelResult(job_id, train_result, test_result, result_detail.model, ModelType.LR_MODEL_SETTING.name)
        return lr_result


class SecureLRPredictJobContext(JobContext):
    def __init__(self, remote_job_client: WeDPRRemoteJobClient, storage_entry_point: StorageEntryPoint, project_id: str, model_setting, predict_algorithm, dataset: DataContext = None, my_agency=None, merge_field: str = 'id'):
        super().__init__(remote_job_client, storage_entry_point, project_id, dataset, my_agency)
        self.model_setting = model_setting
        self.merge_field = merge_field
        self.predict_algorithm = predict_algorithm

    def get_job_type(self) -> JobType:
        return JobType.LR_PREDICTING

    def build(self) -> JobParam:
        self.dataset_list = self.dataset.to_model_formort(
            self.merge_field, self.result_receiver_id_list)
        job_info = JobInfo(job_type=self.get_job_type(), project_id=self.project_id, param=json.dumps(
            {'dataSetList': self.dataset_list, 'modelSetting': self.model_setting, 'modelPredictAlgorithm': json.dumps(self.predict_algorithm)}))
        job_param = JobParam(job_info, self.task_parties, self.dataset_id_list)
        return job_param

    def parse_result(self, job_id, block_until_success):
        result_detail = self.fetch_job_result(job_id, block_until_success)
        test_result = DatasetToolkit(storage_entrypoint=self.storage_entry_point, 
                                    storage_workspace=None, 
                                    dataset_owner=self.storage_entry_point.user_config.user,
                                    dataset_path=result_detail.modelResultDetail['ModelResult']['testResultPath'], agency=self.create_agency)

        lr_result = ModelResult(job_id, test_result=test_result)
        return lr_result
