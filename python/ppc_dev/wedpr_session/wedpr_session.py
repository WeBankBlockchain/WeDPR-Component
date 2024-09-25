from ppc_dev.wedpr_data.data_context import DataContext
from ppc_dev.job_exceuter.pws_client import PWSApi
from ppc_dev.result.psi_result import PSIResult
from ppc_dev.result.fe_result import FeResult
from ppc_dev.result.model_result import ModelResult


class WedprSession:

    def __init__(self, dataset: DataContext = None, my_agency = None):

        self.dataset = dataset
        self.create_agency = my_agency

        if self.dataset is not None:
            self.participant_id_list = self.get_agencies()
            self.label_holder_agency = self.get_label_holder_agency()
        self.result_receiver_id_list = [my_agency]  # 仅限jupyter所在机构
    
        self.excute = PWSApi(self.dataset.ctx.pws_endpoint, self.dataset.ctx.token)

    def task(self, datasets: list, params: dict = {}):

        self.check_agencies()
        job_response = self.excute.run(datasets, params)

        return job_response.job_id

    def psi(self, dataset: DataContext = None, merge_filed: str = 'id'):
        
        if dataset is not None:
            self.update_dataset(dataset)

        # 构造参数
        params = {merge_filed: merge_filed}

        # 执行任务
        job_id = self.task(self.dataset.to_psi_format(), params)

        # 结果处理
        psi_result = PSIResult(dataset, job_id)

        return psi_result

    def proprecessing(self, dataset: DataContext = None, psi_result = None, params: dict = None):

        if dataset is not None:
            self.update_dataset(dataset)

        job_id = self.task(self.dataset.to_model_formort())

        # 结果处理
        datasets_pre = FeResult(dataset, job_id)
        
        return datasets_pre

    def feature_engineering(self, dataset: DataContext = None, psi_result = None, params: dict = None):

        if dataset is not None:
            self.update_dataset(dataset)

        job_id = self.task(self.dataset.to_model_formort())

        # 结果处理
        datasets_fe = FeResult(dataset, job_id)

        return datasets_fe

    def xgb_training(self, dataset: DataContext = None, psi_result = None, params: dict = None):

        if dataset is not None:
            self.update_dataset(dataset)
        self.check_datasets()

        job_id = self.task(self.dataset.to_model_formort())
        
        # 结果处理
        model_result = ModelResult(dataset, job_id, job_type='xgb_training')

        return model_result

    def xgb_predict(self, dataset: DataContext = None, psi_result = None, model = None):

        if dataset is not None:
            self.update_dataset(dataset)
        self.check_datasets()

        # 上传模型到hdfs
        job_id = self.task(self.dataset.to_model_formort())

        # 结果处理
        model_result = ModelResult(dataset, job_id, job_type='xgb_predict')

        # 结果处理
        return model_result

    def update_dataset(self, dataset: DataContext):
        self.dataset = dataset
        self.participant_id_list = self.get_agencies()
        self.label_holder_agency = self.get_label_holder_agency()

    def get_agencies(self):
        participant_id_list = []
        for dataset in self.dataset.datasets:
            participant_id_list.append(dataset.agency.agency_id)
        return participant_id_list

    def get_label_holder_agency(self):
        label_holder_agency = None
        for dataset in self.dataset.datasets:
            if dataset.is_label_holder:
                label_holder_agency = dataset.agency.agency_id
        return label_holder_agency

    def check_agencies(self):
        """
        校验机构数和任务是否匹配
        """
        if len(self.participant_id_list) < 2:
            raise ValueError("至少需要传入两个机构")

    def check_datasets(self):
        """
        校验是否包含标签提供方
        """
        if not self.label_holder_agency or self.label_holder_agency not in self.participant_id_list:
            raise ValueError("数据集中标签提供方配置错误")

    # def get_agencies(self):  
    #     """  
    #     返回所有机构名称的列表。  
          
    #     :return: 机构名称的列表  
    #     """  
    #     return self.agencies  
