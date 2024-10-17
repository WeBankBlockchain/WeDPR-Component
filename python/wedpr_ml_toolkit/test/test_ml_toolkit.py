# -*- coding: utf-8 -*-
import unittest
import numpy as np
import pandas as pd
from sklearn import metrics
from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfigBuilder
from wedpr_ml_toolkit.wedpr_ml_toolkit import WeDPRMlToolkit
from wedpr_ml_toolkit.toolkit.dataset_toolkit import DatasetToolkit
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.config.wedpr_model_setting import PreprocessingModelSetting

wedpr_config = WeDPRMlConfigBuilder.build_from_properties_file(
    "config.properties")

wedpr_ml_toolkit = WeDPRMlToolkit(wedpr_config)

# 注册 dataset，支持两种方式: pd.Dataframe, hdfs_path
df = pd.DataFrame({
    'id': np.arange(0, 100),  # id列，顺序整数
    'y': np.random.randint(0, 2, size=100),
    **{f'x{i}': np.random.rand(100) for i in range(1, 11)}  # x1到x10列，随机数
})

dataset1 = DatasetToolkit(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(),
                          storage_workspace=wedpr_config.user_config.get_workspace_path(),
                          agency=wedpr_config.user_config.agency_name,
                          values=df,
                          is_label_holder=True)
dataset1.save_values(path='d-101')

# hdfs_path
dataset2 = DatasetToolkit(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(),
                          dataset_path="d-9606695119693829", agency="WeBank")

dataset2.storage_client = None
# dataset2.load_values()
if dataset2.storage_client is None:
    # 支持更新dataset的values数据
    df2 = pd.DataFrame({
        'id': np.arange(0, 100),  # id列，顺序整数
        **{f'z{i}': np.random.rand(100) for i in range(1, 11)}  # x1到x10列，随机数
    })
    dataset2.update_values(values=df2)
if dataset1.storage_client is not None:
    dataset1.update_values(
        path='/user/ppc/milestone2/sgd/flyhuang1/d-9606704699156485')
    dataset1.load_values()

# 构建 dataset context
dataset = DataContext(dataset1, dataset2)

# init the job context
project_name = "1"

psi_job_context = wedpr_ml_toolkit.build_job_context(
    JobType.PSI, project_name, dataset, None, "id")
print(psi_job_context.participant_id_list,
      psi_job_context.result_receiver_id_list)
# 执行psi任务
psi_job_id = psi_job_context.submit()
psi_result = psi_job_context.fetch_job_result(psi_job_id, True)

# 初始化
preprocessing_data = DataContext(dataset1)
preprocessing_job_context = wedpr_ml_toolkit.build_job_context(
    JobType.PREPROCESSING, project_name, preprocessing_data, PreprocessingModelSetting())
# 执行预处理任务
fe_job_id = preprocessing_job_context.submit(dataset)
fe_result = preprocessing_job_context.fetch_job_result(fe_job_id, True)
print(preprocessing_job_context.participant_id_list,
      preprocessing_job_context.result_receiver_id_list)
