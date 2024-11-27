#!/usr/bin/env python
# coding: utf-8

# In[4]:


import sys
import numpy as np
import pandas as pd
from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfigBuilder
from wedpr_ml_toolkit.wedpr_ml_toolkit import WeDPRMlToolkit
from wedpr_ml_toolkit.context.dataset_context import DatasetContext
from wedpr_ml_toolkit.transport.wedpr_remote_dataset_client import DatasetMeta


# In[5]:


# 读取配置文件
wedpr_config = WeDPRMlConfigBuilder.build_from_properties_file('config.properties')
wedpr_ml_toolkit = WeDPRMlToolkit(wedpr_config)


# In[8]:


# 注册 dataset，支持两种方式: pd.Dataframe, hdfs_path
# 1. pd.Dataframe
df = pd.DataFrame({
    'id': np.arange(0, 100),  # id列，顺序整数
    'y': np.random.randint(0, 2, size=100),
    # x1到x10列，随机数
    **{f't{i}': np.random.rand(100) for i in range(1, 11)}
})

dataset1_meta = DatasetMeta(file_path = "d-01", user_config = wedpr_config.user_config)

dataset1 = DatasetContext(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(),
                          storage_workspace=wedpr_config.user_config.get_workspace_path(),
                          dataset_meta = dataset1_meta)
print(f"* dataset1: {dataset1.dataset_meta}")
dataset1.save_values(df)
print(f"* updated dataset1: {dataset1}")

(values, cols, shape) = dataset1.load_values()
print(f"* load values result: {cols}, {shape}, {values}")


# In[11]:


# 2. hdfs_path
dataset2 = DatasetContext(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(), 
                          dataset_client=wedpr_ml_toolkit.dataset_client,
                          storage_workspace=wedpr_config.user_config.get_workspace_path(), 
                          dataset_id = "d-9866227816474629")
print(f"* dataset2 meta: {dataset2}")

# load values
(values, cols, shape) = dataset2.load_values(header=0)
print(f"* dataset2 detail, cols: {cols}, shape: {shape}, values: {values}")


# 支持更新dataset的values数据
df2 = pd.DataFrame({
        'id': np.arange(0, 100),  # id列，顺序整数
        **{f'w{i}': np.random.rand(100) for i in range(1, 11)}  # x1到x10列，随机数
    })
dataset2.save_values(values=df2)

print(f"*** updated dataset2 meta: {dataset2}")

(values, cols, shape) = dataset2.load_values(header=0)
print(f"*** updated dataset2 detail, cols: {cols}, shape: {shape}, values: {values}")


# In[ ]:




