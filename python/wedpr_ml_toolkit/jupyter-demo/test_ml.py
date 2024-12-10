#!/usr/bin/env python
# coding: utf-8

# In[1]:


import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, roc_auc_score, precision_recall_curve, accuracy_score, f1_score, precision_score, recall_score
import sys
import numpy as np
import pandas as pd
from wedpr_ml_toolkit.config.wedpr_ml_config import WeDPRMlConfigBuilder
from wedpr_ml_toolkit.wedpr_ml_toolkit import WeDPRMlToolkit
from wedpr_ml_toolkit.context.dataset_context import DatasetContext
from wedpr_ml_toolkit.context.data_context import DataContext
from wedpr_ml_toolkit.context.job_context import JobType
from wedpr_ml_toolkit.context.model_setting import ModelSetting
from wedpr_ml_toolkit.context.result.model_result_context import PredictResultContext
from wedpr_ml_toolkit.context.result.model_result_context import TrainResultContext
from wedpr_ml_toolkit.context.result.model_result_context import PreprocessingResultContext


# In[2]:


# 读取配置文件
wedpr_config = WeDPRMlConfigBuilder.build_from_properties_file(
    'config.properties')
wedpr_ml_toolkit = WeDPRMlToolkit(wedpr_config)


# In[3]:


# dataset1
dataset1 = DatasetContext(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(),
                          dataset_client=wedpr_ml_toolkit.dataset_client,
                          dataset_id='d-9743660607744005',
                          is_label_holder=True)
print(f"* load dataset1: {dataset1}")
(values, cols, shapes) = dataset1.load_values()
print(f"* dataset1 detail: {cols}, {shapes}")
print(f"* dataset1 value: {values}")


# In[4]:


# dataset2
dataset2 = DatasetContext(storage_entrypoint=wedpr_ml_toolkit.get_storage_entry_point(),
                          dataset_client=wedpr_ml_toolkit.dataset_client,
                          dataset_id="d-9743674298214405")
print(f"* dataset2: {dataset2}")

# 构建 dataset context
dataset = DataContext(dataset1, dataset2)
print(dataset.datasets)

# init the job context
project_id = "9737304249804806"

# 构造xgb任务配置
model_setting = ModelSetting()
model_setting.use_psi = True
xgb_job_context = wedpr_ml_toolkit.build_job_context(
    job_type=JobType.XGB_TRAINING,
    project_id=project_id,
    dataset=dataset,
    model_setting=model_setting)
print(f"* build xgb job context: {xgb_job_context}")


# In[5]:


# 执行xgb任务
xgb_job_id = xgb_job_context.submit()
print(xgb_job_id)


# In[7]:


# 获取xgb任务结果
print(xgb_job_id)
# xgb_job_id = "9868279583877126"
xgb_result_detail = xgb_job_context.fetch_job_result(xgb_job_id, True)
# load the result context
xgb_result_context = wedpr_ml_toolkit.build_result_context(job_context=xgb_job_context,
                                                           job_result_detail=xgb_result_detail)
print(f"* xgb job result ctx: {xgb_result_context}")

xgb_test_dataset = xgb_result_context.test_result_dataset
print(
    f"* xgb_test_dataset: {xgb_test_dataset}, file_path: {xgb_test_dataset.dataset_meta.file_path}")

(data, cols, shapes) = xgb_test_dataset.load_values()
print(
    f"* test dataset detail, columns: {cols}, shape: {shapes}, value: {data}")


# In[8]:


# evaluation result
result_context: TrainResultContext = xgb_result_context
evaluation_result_dataset = result_context.evaluation_dataset
(eval_data, cols, shape) = evaluation_result_dataset.load_values(header=0)
print(
    f"* evaluation detail, col: {cols}, shape: {shape}, eval_data: {eval_data}")


# In[9]:


# feature importance
feature_importance_dataset = result_context.feature_importance_dataset
(feature_importance_data, cols, shape) = feature_importance_dataset.load_values()

print(
    f"* feature_importance detail, col: {cols}, shape: {shape}, feature_importance_data: {feature_importance_data}")


# In[10]:


# 预处理结果
preprocessing_dataset = result_context.preprocessing_dataset
(preprocessing_data, cols, shape) = preprocessing_dataset.load_values()

print(
    f"* preprocessing detail, col: {cols}, shape: {shape}, preprocessing_data: {preprocessing_data}")


# In[11]:


# 建模结果
model_result_dataset = result_context.model_result_dataset
(model_result, cols, shape) = model_result_dataset.load_values()

print(
    f"* model_result detail, col: {cols}, shape: {shape}, model_result: {model_result}")


# In[12]:


# 明文处理预测结果

# 提取真实标签和预测概率
y_true = data['class_label']
y_pred_proba = data['class_pred']
y_pred = np.where(y_pred_proba >= 0.5, 1, 0)  # 二分类阈值设为0.5

# 计算评估指标
accuracy = accuracy_score(y_true, y_pred)
precision = precision_score(y_true, y_pred)
recall = recall_score(y_true, y_pred)
f1 = f1_score(y_true, y_pred)
auc = roc_auc_score(y_true, y_pred_proba)

print(f"Accuracy: {accuracy:.2f}")
print(f"Precision: {precision:.2f}")
print(f"Recall: {recall:.2f}")
print(f"F1 Score: {f1:.2f}")
print(f"AUC: {auc:.2f}")

# ROC 曲线
fpr, tpr, _ = roc_curve(y_true, y_pred_proba)
plt.figure(figsize=(12, 5))

# ROC 曲线
plt.subplot(1, 2, 1)
plt.plot(fpr, tpr, label=f'AUC = {auc:.2f}')
plt.plot([0, 1], [0, 1], 'k--')
plt.xlabel('False Positive Rate')
plt.ylabel('True Positive Rate')
plt.title('ROC Curve')
plt.legend()

# 精确率-召回率曲线
precision_vals, recall_vals, _ = precision_recall_curve(y_true, y_pred_proba)
plt.subplot(1, 2, 2)
plt.plot(recall_vals, precision_vals)
plt.xlabel('Recall')
plt.ylabel('Precision')
plt.title('Precision-Recall Curve')

plt.tight_layout()
plt.show()


# In[13]:


# 构造xgb预测任务配置
predict_setting = ModelSetting()
predict_setting.use_psi = True
# model_predict_algorithm = {}
# model_predict_algorithm.update({"setting": xgb_result_context.job_result_detail.model})
predict_xgb_job_context = wedpr_ml_toolkit.build_job_context(
    job_type=JobType.XGB_PREDICTING,
    project_id=project_id,
    dataset=dataset,
    model_setting=predict_setting,
    predict_algorithm=xgb_result_context.job_result_detail.model_predict_algorithm)
print(f"* predict_xgb_job_context: {predict_xgb_job_context}")


# In[14]:


# 执行xgb预测任务
# xgb_job_id = '9868428439267334'  # 测试时跳过创建新任务过程
xgb_predict_job_id = predict_xgb_job_context.submit()
print(xgb_predict_job_id)


# In[15]:


# query the job detail
print(f"* xgb_predict_job_id: {xgb_predict_job_id}")

predict_xgb_job_result = predict_xgb_job_context.fetch_job_result(
    xgb_predict_job_id, True)

# generate the result context
result_context = wedpr_ml_toolkit.build_result_context(job_context=predict_xgb_job_context,
                                                       job_result_detail=predict_xgb_job_result)

xgb_predict_result_context: PredictResultContext = result_context
print(f"* result_context is {xgb_predict_result_context}")


# In[16]:


# 明文处理预测结果


(data, cols, shapes) = xgb_predict_result_context.model_result_dataset.load_values(header=0)

# 提取真实标签和预测概率
y_true = data['class_label']
y_pred_proba = data['class_pred']
y_pred = np.where(y_pred_proba >= 0.5, 1, 0)  # 二分类阈值设为0.5

# 计算评估指标
accuracy = accuracy_score(y_true, y_pred)
precision = precision_score(y_true, y_pred)
recall = recall_score(y_true, y_pred)
f1 = f1_score(y_true, y_pred)
auc = roc_auc_score(y_true, y_pred_proba)

print(f"Accuracy: {accuracy:.2f}")
print(f"Precision: {precision:.2f}")
print(f"Recall: {recall:.2f}")
print(f"F1 Score: {f1:.2f}")
print(f"AUC: {auc:.2f}")

# ROC 曲线
fpr, tpr, _ = roc_curve(y_true, y_pred_proba)
plt.figure(figsize=(12, 5))

# ROC 曲线
plt.subplot(1, 2, 1)
plt.plot(fpr, tpr, label=f'AUC = {auc:.2f}')
plt.plot([0, 1], [0, 1], 'k--')
plt.xlabel('False Positive Rate')
plt.ylabel('True Positive Rate')
plt.title('ROC Curve')
plt.legend()

# 精确率-召回率曲线
precision_vals, recall_vals, _ = precision_recall_curve(y_true, y_pred_proba)
plt.subplot(1, 2, 2)
plt.plot(recall_vals, precision_vals)
plt.xlabel('Recall')
plt.ylabel('Precision')
plt.title('Precision-Recall Curve')

plt.tight_layout()
plt.show()


# In[ ]:
