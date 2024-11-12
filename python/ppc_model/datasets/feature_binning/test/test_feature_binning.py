import unittest

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext


data_size = 1000
feature_dim = 100
has_label = True


class TestFeatureBinning(unittest.TestCase):

    components = Initializer(log_config_path='', config_path='')
    components.config_data = {'JOB_TEMP_DIR': '/tmp'}
    components.mock_logger = MockLogger()

    def test_train_feature_binning(self):

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
                'objective': 'regression',
                'max_bin': 10,
                'n_estimators': 6,
                'max_depth': 3,
                'use_goss': 1
            }
        }

        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label)
        secure_dataset = SecureDataset(task_info, model_data)
        print(secure_dataset.train_idx.shape)
        print(secure_dataset.train_X.shape)
        print(secure_dataset.train_y.shape)
        print(secure_dataset.test_idx.shape)
        print(secure_dataset.test_X.shape)
        print(secure_dataset.test_y.shape)

        feat_bin = FeatureBinning(task_info)
        data_bin, data_split = feat_bin.data_binning(secure_dataset.train_X)

        self.assertEqual(data_bin.shape, secure_dataset.train_X.shape)
        # print(data_bin)
        # print(data_split)

    def test_test_feature_binning(self):

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
                'objective': 'regression',
                'categorical_feature': [],
                'max_bin': 10,
                'n_estimators': 6,
                'max_depth': 3,
                'use_goss': 1
            }
        }

        task_info = SecureLGBMContext(args['task_id'], args, self.components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label)
        secure_dataset = SecureDataset(task_info, model_data)
        print(secure_dataset.train_idx.shape)
        print(secure_dataset.train_X.shape)
        print(secure_dataset.train_y.shape)
        print(secure_dataset.test_idx.shape)
        print(secure_dataset.test_X.shape)
        print(secure_dataset.test_y.shape)

        feat_bin = FeatureBinning(task_info)
        data_split = None
        data_bin, data_split = feat_bin.data_binning(
            secure_dataset.train_X, data_split)

        self.assertEqual(data_bin.shape, secure_dataset.train_X.shape)
        # print(data_bin)
        # print(data_split)


if __name__ == "__main__":
    unittest.main()
