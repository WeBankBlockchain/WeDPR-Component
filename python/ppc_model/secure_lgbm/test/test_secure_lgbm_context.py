import unittest

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext


class TestSecureLGBMContext(unittest.TestCase):

    components = Initializer(log_config_path='', config_path='')
    components.config_data = {'JOB_TEMP_DIR': '/tmp'}
    components.mock_logger = MockLogger()

    def test_get_lgbm_params(self):

        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': None,
            'algorithm_subtype': None,
            'model_dict': {}
        }

        task_info = SecureLGBMContext(args, self.components)
        lgbm_params = task_info.get_lgbm_params()
        # 打印LGBMModel默认参数
        print(lgbm_params._get_params())

        # 默认自定义参数为空字典
        assert lgbm_params.get_params() == {}
        # assert lgbm_params.get_all_params() != lgbm_params._get_params()

    def test_set_lgbm_params(self):

        args = {
            'job_id': 'j-123',
            'task_id': '1',
            'is_label_holder': True,
            'result_receiver_id_list': [],
            'participant_id_list': [],
            'model_predict_algorithm': None,
            'algorithm_type': None,
            'algorithm_subtype': None,
            'model_dict': {
                'objective': 'regression',
                'n_estimators': 6,
                'max_depth': 3,
                'test_size': 0.2,
                'use_goss': 1
            }
        }

        task_info = SecureLGBMContext(args, self.components)
        lgbm_params = task_info.get_lgbm_params()
        # 打印SecureLGBMParams自定义参数
        print(lgbm_params.get_params())
        # 打印SecureLGBMParams所有参数
        print(lgbm_params.get_all_params())

        assert lgbm_params.get_params() == args['model_dict']
        self.assertEqual(lgbm_params.get_all_params()[
                         'learning_rate'], lgbm_params._get_params()['learning_rate'])
        self.assertEqual(lgbm_params.learning_rate,
                         lgbm_params._get_params()['learning_rate'])
        self.assertEqual(lgbm_params.n_estimators,
                         args['model_dict']['n_estimators'])
        self.assertEqual(lgbm_params.test_size,
                         args['model_dict']['test_size'])
        self.assertEqual(lgbm_params.use_goss, args['model_dict']['use_goss'])


if __name__ == "__main__":
    unittest.main()
