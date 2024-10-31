import unittest
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.datasets import load_breast_cancer

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext
from ppc_model.datasets.dataset import SecureDataset


class TestSecureDataset(unittest.TestCase):
    cancer = load_breast_cancer()
    X = cancer.data
    y = cancer.target

    df = SecureDataset.assembling_dataset(X, y)
    assert (df.columns == ['id', 'y'] + [f'x{i + 1}' for i in range(30)]).all()

    # 模拟生成主动方和被动方数据集
    df_with_y, df_without_y = SecureDataset.hetero_split_dataset(df)
    assert (df_with_y.columns == ['id', 'y'] +
            [f'x{i + 1}' for i in range(15)]).all()
    assert (df_without_y.columns == ['id'] +
            [f'x{i + 16}' for i in range(15)]).all()

    # 模拟自定义分组
    import os

    eval_column_file = './model_eval_column.csv'
    if not os.path.exists(eval_column_file):
        # 创建一个包含569行，2列的数据，其中415个为'INS'，154个为'OOS'
        group_set = np.concatenate([['INS'] * 415, ['OOS'] * 154])
        np.random.shuffle(group_set)
        eval_set_df = pd.DataFrame(
            {'id': np.arange(1, 570), 'group': group_set})
        eval_set_df.to_csv(eval_column_file, index=None)

    df_with_y_file = './df_with_y.csv'
    if not os.path.exists(df_with_y_file):
        df_with_y.to_csv(df_with_y_file, index=None, sep=' ')

    df_without_y_file = './df_without_y.csv'
    if not os.path.exists(df_without_y_file):
        df_without_y.to_csv(df_without_y_file, index=None, sep=' ')

    iv_selected_file = './iv_selected.csv'
    if not os.path.exists(iv_selected_file):
        iv_selected = pd.DataFrame(
            {'feature': [f'x{i + 1}' for i in range(30)],
             'iv_selected': np.random.binomial(n=1, p=0.5, size=30)})
        iv_selected.to_csv(iv_selected_file, index=None)

    components = Initializer(log_config_path='', config_path='')
    components.config_data = {'JOB_TEMP_DIR': '/tmp', 'MAX_THREAD_WORKERS': 10}
    components.mock_logger = MockLogger()

    def test_random_split_dataset(self):

        # 构造主动方参数配置
        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': 'Train',
            'algorithm_subtype': None,
            'model_dict': {
                'random_state': 2024
            }
        }
        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        print(task_info.model_params.get_all_params())

        # 模拟构造主动方数据集
        dataset_with_y = SecureDataset(task_info, self.df_with_y)
        assert (dataset_with_y.train_idx ==
                train_test_split(np.array(range(1, 570)), test_size=0.3, random_state=2024)[0]).all()
        self.assertEqual(dataset_with_y.train_X.shape, (398, 15))
        self.assertEqual(dataset_with_y.test_X.shape, (171, 15))
        self.assertEqual(dataset_with_y.train_y.shape, (398,))
        self.assertEqual(dataset_with_y.test_y.shape, (171,))

        # 构造被动方参数配置
        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': False,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': 'Train',
            'algorithm_subtype': None,
            'model_dict': {
                'random_state': 2024
            }
        }
        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        print(task_info.model_params.get_all_params())

        # 模拟构造被动方数据集
        dataset_without_y = SecureDataset(task_info, self.df_without_y)
        assert (dataset_without_y.train_idx == dataset_with_y.train_idx).all()
        self.assertEqual(dataset_without_y.train_X.shape, (398, 15))
        self.assertEqual(dataset_without_y.test_X.shape, (171, 15))
        self.assertEqual(dataset_without_y.train_y, None)
        self.assertEqual(dataset_without_y.test_y, None)

    def test_customized_split_dataset(self):

        # 构造主动方参数配置
        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': 'Train',
            'algorithm_subtype': None,
            'model_dict': {
                'eval_set_column': 'group',
                'train_set_value': 'INS',
                'eval_set_value': 'OOS'
            }
        }
        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        print(task_info.model_params.get_all_params())

        # 模拟构造主动方数据集
        task_info.eval_column_file = self.eval_column_file
        task_info.model_prepare_file = self.df_with_y_file
        eval_set_df = pd.read_csv(task_info.eval_column_file, header=0)

        dataset_with_y = SecureDataset(task_info)
        assert (dataset_with_y.train_idx ==
                eval_set_df['id'][eval_set_df['group'] == 'INS']).all()
        self.assertEqual(dataset_with_y.train_X.shape, (415, 15))
        self.assertEqual(dataset_with_y.test_X.shape, (154, 15))
        self.assertEqual(dataset_with_y.train_y.shape, (415,))
        self.assertEqual(dataset_with_y.test_y.shape, (154,))

    def test_predict_dataset(self):

        # 构造主动方参数配置
        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': 'Predict',
            'algorithm_subtype': None,
            'model_dict': {}
        }
        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        print(task_info.model_params.get_all_params())

        # 模拟构造主动方数据集
        task_info.model_prepare_file = self.df_with_y_file
        dataset_with_y = SecureDataset(task_info)

        self.assertEqual(dataset_with_y.train_X, None)
        self.assertEqual(dataset_with_y.test_X.shape, (569, 15))
        self.assertEqual(dataset_with_y.train_y, None)
        self.assertEqual(dataset_with_y.test_y.shape, (569,))

    def test_iv_selected_dataset(self):

        # 构造主动方参数配置
        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': 'Predict',
            'algorithm_subtype': None,
            'model_dict': {}
        }
        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        print(task_info.model_params.get_all_params())

        # 模拟构造主动方数据集
        task_info.model_prepare_file = self.df_with_y_file
        task_info.iv_selected_file = './iv_selected.csv'
        dataset_with_y = SecureDataset(task_info)

        self.assertEqual(dataset_with_y.train_X, None)
        self.assertEqual(dataset_with_y.test_X.shape, (569, 9))
        self.assertEqual(dataset_with_y.train_y, None)
        self.assertEqual(dataset_with_y.test_y.shape, (569,))

    def test_read_dataset(self):
        np.random.seed(0)
        origin_data = np.random.randint(0, 100, size=(100, 10))
        columns = ['id'] + [f"x{i}" for i in range(2, 11)]
        df = pd.DataFrame(origin_data, columns=columns)
        csv_file = '/tmp/data_x1_to_x10.csv'
        df.to_csv(csv_file, index=False)
        field_list, label, feature = SecureDataset.read_dataset(
            csv_file, False, delimiter=',')
        self.assertEqual(['id'] + field_list, columns)
        field_list, label, feature = SecureDataset.read_dataset(
            csv_file, True, delimiter=',')
        self.assertEqual(['id'] + field_list, columns)


if __name__ == "__main__":
    unittest.main()
