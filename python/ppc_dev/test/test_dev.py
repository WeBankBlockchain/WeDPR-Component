import unittest
import numpy as np
import pandas as pd
from sklearn import metrics

from ppc_dev.common.base_context import BaseContext
from ppc_dev.utils.agency import Agency
from ppc_dev.wedpr_data.wedpr_data import WedprData
from ppc_dev.wedpr_data.data_context import DataContext
from ppc_dev.wedpr_session.wedpr_session import WedprSession


# 从jupyter环境中获取project_id等信息
# create workspace
# 相同项目/刷新专家模式project_id固定
project_id = 'p-123'
user = 'admin'
my_agency='WeBank'
pws_endpoint = '0.0.0.0:0000'
hdfs_endpoint = '0.0.0.0:0001'
token = 'abc...'


# 自定义合作方机构
partner_agency1='SG'
partner_agency2='TX'

# 初始化project ctx 信息
ctx = BaseContext(project_id, user, pws_endpoint, hdfs_endpoint, token)

# 注册 agency
agency1 = Agency(agency_id=my_agency)
agency2 = Agency(agency_id=partner_agency1)

# 注册 dataset，支持两种方式: pd.Dataframe, hdfs_path
# pd.Dataframe
df = pd.DataFrame({
    'id': np.arange(0, 100),  # id列，顺序整数
    **{f'x{i}': np.random.rand(100) for i in range(1, 11)}  # x1到x10列，随机数
})
dataset1 = WedprData(ctx, values=df, agency=agency1)
dataset1.storage_client = None
dataset1.save_values(path='./project_id/user/data/d-101')
# hdfs_path
dataset2 = WedprData(ctx, dataset_path='./data_path/d-123', agency=agency2, is_label_holder=True)
dataset2.storage_client = None
dataset2.load_values()

# 支持更新dataset的values数据
df2 = pd.DataFrame({
    'id': np.arange(0, 100),  # id列，顺序整数
    'y': np.random.randint(0, 2, size=100),
    **{f'x{i}': np.random.rand(100) for i in range(1, 11)}  # x1到x10列，随机数
})
dataset2.update_values(values=df2)

# 构建 dataset context
dataset = DataContext(dataset1, dataset2)

# 初始化 wedpr task session（含数据）
task = WedprSession(dataset, my_agency=my_agency)
print(task.participant_id_list, task.result_receiver_id_list)
# 执行psi任务
psi_result = task.psi()

# 初始化 wedpr task session（不含数据）  （推荐：使用更灵活）
task = WedprSession(my_agency=my_agency)
# 执行psi任务
fe_result = task.proprecessing(dataset)
print(task.participant_id_list, task.result_receiver_id_list)