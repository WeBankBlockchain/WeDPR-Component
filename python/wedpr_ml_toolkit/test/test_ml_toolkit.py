# -*- coding: utf-8 -*-
import unittest
import numpy as np
import pandas as pd
from sklearn import metrics
from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfigBuilder
from wedpr_ml_toolkit.wedpr_ml_toolkit import WeDPRMlToolkit
from wedpr_ml_toolkit.context.dataset_context import DatasetContext
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.context.model_setting import PreprocessingSetting


class WeDPRMlToolkitTestWrapper:
    def __init__(self, config_file_path):
        self.wedpr_config = WeDPRMlConfigBuilder.build_from_properties_file(
            config_file_path)
        self.wedpr_ml_toolkit = WeDPRMlToolkit(self.wedpr_config)

    def test_submit_job(self):
        # 注册 dataset，支持两种方式: pd.Dataframe, hdfs_path
        df = pd.DataFrame({
            'id': np.arange(0, 100),  # id列，顺序整数
            'y': np.random.randint(0, 2, size=100),
            # x1到x10列，随机数
            **{f'x{i}': np.random.rand(100) for i in range(1, 11)}
        })
        dataset1 = DatasetContext(storage_entrypoint=self.wedpr_ml_toolkit.get_storage_entry_point(),
                                  dataset_client=self.wedpr_ml_toolkit.get_dataset_client(),
                                  storage_workspace=self.wedpr_config.user_config.get_workspace_path(),
                                  dataset_id="d-9743660607744005",
                                  is_label_holder=True)
        dataset1.save_values(df, path='d-101')

        # hdfs_path
        dataset2 = DatasetContext(storage_entrypoint=self.wedpr_ml_toolkit.get_storage_entry_point(),
                                  dataset_client=self.wedpr_ml_toolkit.get_dataset_client(),
                                  dataset_id="d-9743674298214405")

        dataset2.storage_client = None
        # dataset2.load_values()
        if dataset2.storage_client is None:
            # 支持更新dataset的values数据
            df2 = pd.DataFrame({
                'id': np.arange(0, 100),  # id列，顺序整数
                # x1到x10列，随机数
                **{f'z{i}': np.random.rand(100) for i in range(1, 11)}
            })
            dataset2.save_values(values=df2)
        if dataset1.storage_client is not None:
            # save values to dataset1
            dataset1.save_values(df)
            (values, columns, shape) = dataset1.load_values()
            print(f"### values: {values}")

        # 构建 dataset context
        dataset = DataContext(dataset1, dataset2)

        # init the job context
        project_id = "9737304249804806"
        print("* build psi job context")
        psi_job_context = self.wedpr_ml_toolkit.build_job_context(
            JobType.PSI, project_id, dataset, None, "id")
        print(psi_job_context.participant_id_list,
              psi_job_context.result_receiver_id_list)
        # 执行psi任务
        print("* submit psi job")
        psi_job_id = psi_job_context.submit()
        print(f"* submit psi job success, job_id: {psi_job_id}")
        psi_result = psi_job_context.fetch_job_result(psi_job_id, True)
        print(
            f"* fetch_job_result for psi job {psi_job_id} success, result: {psi_result}")
        # 初始化
        print(f"* build pre-processing data-context")
        preprocessing_data = DataContext(dataset1, dataset2)
        preprocessing_job_context = self.wedpr_ml_toolkit.build_job_context(
            JobType.PREPROCESSING, project_id, preprocessing_data, PreprocessingSetting())
        # 执行预处理任务
        print(f"* submit pre-processing job")
        fe_job_id = preprocessing_job_context.submit()
        print(f"* submit pre-processing job success, job_id: {fe_job_id}")
        fe_result = preprocessing_job_context.fetch_job_result(fe_job_id, True)
        print(
            f"* fetch pre-processing job result success, job_id: {fe_job_id}, result: {fe_result}")
        print(preprocessing_job_context.participant_id_list,
              preprocessing_job_context.result_receiver_id_list)

    def test_query_job(self, job_id: str, block_until_finish):
        job_result = self.wedpr_ml_toolkit.query_job_status(
            job_id, block_until_finish)
        print(f"#### query_job_status job_result: {job_result}")
        job_detail_result = self.wedpr_ml_toolkit.query_job_detail(
            job_id, block_until_finish)
        print(f"#### query_job_detail job_detail_result: {job_detail_result}")
        return (job_result, job_detail_result)


class TestMlToolkit(unittest.TestCase):
    def test_query_jobs(self):
        wrapper = WeDPRMlToolkitTestWrapper("config.properties")
        job_id = wrapper.test_submit_job()


if __name__ == '__main__':
    unittest.main()
