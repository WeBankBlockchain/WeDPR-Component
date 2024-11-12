from sklearn.preprocessing import StandardScaler
from sklearn.preprocessing import MinMaxScaler
import unittest
import threading
import traceback
import numpy as np
from sklearn.datasets import load_breast_cancer

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.metrics.evaluation import Evaluation
from ppc_model.metrics.loss import BinaryLoss
from ppc_model.secure_lr.secure_lr_context import SecureLRContext
from ppc_model.secure_lr.vertical.booster import VerticalBooster


ACTIVE_PARTY = 'ACTIVE_PARTY'
PASSIVE_PARTY = 'PASSIVE_PARTY'


def mock_args():
    job_id = 'j-1234'
    task_id = 't-1234'

    model_dict = {
        'objective': 'regression',
        'categorical_feature': [],
        'train_features': "",
        'epochs': 1,
        'batch_size': 8,
        'feature_rate': 0.8,
        'random_state': 2024
    }

    args_a = {
        'job_id': job_id,
        'task_id': task_id,
        'is_label_holder': True,
        'result_receiver_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'participant_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'model_predict_algorithm': None,
        'algorithm_type': 'Train',
        'algorithm_subtype': 'HeteroXGB',
        'model_dict': model_dict
    }

    args_b = {
        'job_id': job_id,
        'task_id': task_id,
        'is_label_holder': False,
        'result_receiver_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'participant_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'model_predict_algorithm': None,
        'algorithm_type': 'Train',
        'algorithm_subtype': 'HeteroXGB',
        'model_dict': model_dict
    }

    return args_a, args_b


cancer = load_breast_cancer()
X = cancer.data
y = cancer.target

df = SecureDataset.assembling_dataset(X, y)
df_with_y, df_without_y = SecureDataset.hetero_split_dataset(df)

args_a, args_b = mock_args()

active_components = Initializer(log_config_path='', config_path='')
active_components.config_data = {
    'JOB_TEMP_DIR': '/tmp/active', 'AGENCY_ID': ACTIVE_PARTY}
active_components.mock_logger = MockLogger()
task_info_a = SecureLRContext(args_a['task_id'], args_a, active_components)

# df ---------------------------------------------
secure_dataset = SecureDataset(task_info_a, df)
max_iter = VerticalBooster._init_iter(
    secure_dataset.train_X.shape[0], 3, 8)

train_praba = VerticalBooster._init_praba(secure_dataset.train_X.shape[0])
train_weights = VerticalBooster._init_weight(secure_dataset.train_X.shape[1])
bias = 0

# for _ in range(max_iter):
for i in range(1):
    idx = VerticalBooster._get_sample_idx(
        i, secure_dataset.train_X.shape[0], size=8)
    x_, y_ = secure_dataset.train_X[idx], secure_dataset.train_y[idx]

    g = BinaryLoss.dot_product(x_, train_weights) + bias
    # h = 0.5 + BinaryLoss.inference(g)
    h = BinaryLoss.sigmoid(g)
    d = h - y_
    deriv = np.matmul(x_.T, d) / x_.shape[0]
    deriv_bias = np.sum(d) / x_.shape[0]
    print(deriv)

    train_weights -= 0.1 * deriv.astype('float')
    bias -= 0.1 * deriv_bias
    print(train_weights)
    print(bias)

g = BinaryLoss.dot_product(secure_dataset.train_X, train_weights) + bias
train_praba = BinaryLoss.sigmoid(g)
auc = Evaluation.fevaluation(secure_dataset.train_y, train_praba)['auc']
print(auc)


# df ---------------------------------------------
# not bias
train_praba = VerticalBooster._init_praba(secure_dataset.train_X.shape[0])
train_weights = VerticalBooster._init_weight(secure_dataset.train_X.shape[1])

# for _ in range(max_iter):
for i in range(2):
    idx = VerticalBooster._get_sample_idx(
        i, secure_dataset.train_X.shape[0], size=8)
    x_, y_ = secure_dataset.train_X[idx], secure_dataset.train_y[idx]

    g = BinaryLoss.dot_product(x_, train_weights)
    h = 0.5 + BinaryLoss.inference(g)
    # h = BinaryLoss.sigmoid(g)
    d = h - y_
    deriv = np.matmul(x_.T, d) / x_.shape[0]
    print(deriv)

    train_weights -= 0.1 * deriv.astype('float')
    print(train_weights)

g = BinaryLoss.dot_product(secure_dataset.train_X, train_weights)
# train_praba = 0.5 + BinaryLoss.inference(g)
train_praba = BinaryLoss.sigmoid(g)
auc = Evaluation.fevaluation(secure_dataset.train_y, train_praba)['auc']
print(auc)


# df ---------------------------------------------
# MinMaxScaler

# 创建MinMaxScaler对象
scaler = MinMaxScaler()

# 拟合并转换数据
train_X = scaler.fit_transform(secure_dataset.train_X)

train_praba = VerticalBooster._init_praba(secure_dataset.train_X.shape[0])
train_weights = VerticalBooster._init_weight(secure_dataset.train_X.shape[1])

for i in range(2):
    idx = VerticalBooster._get_sample_idx(i, train_X.shape[0], size=8)
    x_, y_ = train_X[idx], secure_dataset.train_y[idx]

    g = BinaryLoss.dot_product(x_, train_weights)
    h = 0.5 + BinaryLoss.inference(g)
    # h = BinaryLoss.sigmoid(g)
    d = h - y_
    deriv = np.matmul(x_.T, d) / x_.shape[0]
    print(deriv)

    train_weights -= 0.1 * deriv.astype('float')
    print(train_weights)

g = BinaryLoss.dot_product(train_X, train_weights)
# train_praba = 0.5 + BinaryLoss.inference(g)
train_praba = BinaryLoss.sigmoid(g)
auc = Evaluation.fevaluation(secure_dataset.train_y, train_praba)['auc']
print(auc)


# StandardScaler

# 创建MinMaxScaler对象
scaler = StandardScaler()

# 拟合并转换数据
train_X = scaler.fit_transform(secure_dataset.train_X)

train_praba = VerticalBooster._init_praba(secure_dataset.train_X.shape[0])
train_weights = VerticalBooster._init_weight(secure_dataset.train_X.shape[1])

for i in range(2):
    idx = VerticalBooster._get_sample_idx(i, train_X.shape[0], size=8)
    x_, y_ = train_X[idx], secure_dataset.train_y[idx]

    g = BinaryLoss.dot_product(x_, train_weights)
    h = 0.5 + BinaryLoss.inference(g)
    # h = BinaryLoss.sigmoid(g)
    d = h - y_
    deriv = np.matmul(x_.T, d) / x_.shape[0]
    print(deriv)

    train_weights -= 0.1 * deriv.astype('float')
    print(train_weights)

g = BinaryLoss.dot_product(train_X, train_weights)
# train_praba = 0.5 + BinaryLoss.inference(g)
train_praba = BinaryLoss.sigmoid(g)
auc = Evaluation.fevaluation(secure_dataset.train_y, train_praba)['auc']
print(auc)
