# -*- coding: utf-8 -*-
import unittest
import numpy as np
import pandas as pd
from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfigBuilder
from wedpr_ml_toolkit.wedpr_ml_toolkit import WeDPRMlToolkit
from wedpr_ml_toolkit.context.dataset_context import DatasetContext
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.context.model_setting import PreprocessingSetting
from wedpr_ml_toolkit.context.model_setting import ModelSetting


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
        # the dataset
        dataset1 = DatasetContext(storage_entrypoint=self.wedpr_ml_toolkit.get_storage_entry_point(),
                                  dataset_client=self.wedpr_ml_toolkit.get_dataset_client(),
                                  storage_workspace=self.wedpr_config.user_config.get_workspace_path(),
                                  dataset_id="d-9743660607744005",
                                  is_label_holder=True)
        dataset1.save_values(df, path='d-101')

        # the dataset
        dataset2 = DatasetContext(storage_entrypoint=self.wedpr_ml_toolkit.get_storage_entry_point(),
                                  dataset_client=self.wedpr_ml_toolkit.get_dataset_client(),
                                  dataset_id="d-9743674298214405")
        print(f"### dataset2 meta: {dataset2.dataset_meta}")
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
        # build the psi result:
        psi_result_ctx = self.wedpr_ml_toolkit.build_result_context(
            psi_job_context, psi_result)
        print(f"* psi_result_ctx: {psi_result_ctx}")
        (psi_result_values, psi_result_columns,
         psi_result_shape) = psi_result_ctx.result_dataset.load_values()
        # obtain the intersection
        print(
            f"* psi result, psi_result_columns: {psi_result_columns}, "
            f"psi_result_shape: {psi_result_shape}, psi_result_values: {psi_result_values}")
        # 初始化
        print(f"* build pre-processing data-context")
        preprocessing_data = DataContext(dataset1, dataset2)
        preprocessing_job_context = self.wedpr_ml_toolkit.build_job_context(
            JobType.PREPROCESSING, project_id, preprocessing_data, PreprocessingSetting())
        # 执行预处理任务
        print(f"* submit pre-processing job")
        preprocessing_job_id = preprocessing_job_context.submit()
        print(
            f"* submit pre-processing job success, job_id: {preprocessing_job_id}")
        preprocessing_result = preprocessing_job_context.fetch_job_result(
            preprocessing_job_id, True)
        print(
            f"* fetch pre-processing job result success, job_id: {preprocessing_job_id}, result: {preprocessing_result}")
        print(preprocessing_job_context.participant_id_list,
              preprocessing_job_context.result_receiver_id_list)
        # build the context
        preprocessing_result_ctx = self.wedpr_ml_toolkit.build_result_context(preprocessing_job_context,
                                                                              preprocessing_result)
        print(
            f"* preprocessing_result_ctx: {preprocessing_result_ctx.preprocessing_dataset}")
        preprocessing_values, columns, shape = preprocessing_result_ctx.preprocessing_dataset.load_values()
        print(
            f"* preprocessing_result_dataset, columns: {columns}, shape: {shape}")
        # test xgb job
        xgb_data = DataContext(dataset1, dataset2)
        model_setting = ModelSetting()
        model_setting.use_psi = True
        xgb_job_context = self.wedpr_ml_toolkit.build_job_context(
            job_type=JobType.XGB_TRAINING, project_id=project_id,
            dataset=xgb_data,
            model_setting=model_setting, id_fields="id")
        print(f"* construct xgb job context: participant_id_list: {xgb_job_context.participant_id_list}, "
              f"result_receiver_id_list: {xgb_job_context.result_receiver_id_list}")
        xgb_job_id = xgb_job_context.submit()
        print(f"* submit xgb job success, {xgb_job_id}")
        xgb_job_result = xgb_job_context.fetch_job_result(xgb_job_id, True)
        print(f"* xgb job result: {xgb_job_result}")
        xgb_job_context = self.wedpr_ml_toolkit.build_result_context(
            job_context=xgb_job_context, job_result_detail=xgb_job_result)
        print(f"* xgb job result: {xgb_job_context}")
        # load the feature_importance information
        (feature_importance_value, feature_importance_cols, feature_importance_shape) = \
            xgb_job_context.feature_importance_dataset.load_values()
        print(f"* xgb feature importance information: {feature_importance_cols}, "
              f"{feature_importance_shape}, {feature_importance_value}")
        # load the evaluation information
        (evaluation_value, evaluation_cols, evaluation_shape) = \
            xgb_job_context.evaluation_dataset.load_values()
        print(f"* xgb evaluation information: {evaluation_cols}, "
              f"{evaluation_shape}, {evaluation_value}")

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
