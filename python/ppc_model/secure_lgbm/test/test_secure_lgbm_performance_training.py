import unittest
import threading
import traceback

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.metrics.evaluation import Evaluation
from ppc_model.metrics.model_plot import ModelPlot
from ppc_model.common.model_result import ResultFileHandling
from ppc_model.common.mock.mock_model_transport import MockModelRouterApi
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext
from ppc_model.secure_lgbm.vertical import VerticalLGBMActiveParty, VerticalLGBMPassiveParty


ACTIVE_PARTY = 'ACTIVE_PARTY'
PASSIVE_PARTY = 'PASSIVE_PARTY'

data_size = 1000
feature_dim = 20


def mock_args():
    job_id = 'j-111'
    task_id = 't-111'

    model_dict = {
        'objective': 'regression',
        'categorical_feature': [],
        'train_features': "",
        'max_bin': 10,
        'n_estimators': 2,
        'max_depth': 3,
        'feature_rate': 1.0,
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


class TestXgboostTraining(unittest.TestCase):

    def setUp(self):
        participants = [PASSIVE_PARTY, ACTIVE_PARTY]
        self._active_transport = MockModelRouterApi(participants)
        self._passive_transport = MockModelRouterApi(participants)

    def test_fit(self):
        args_a, args_b = mock_args()

        active_components = Initializer(log_config_path='', config_path='')
        active_components.transport = self._active_transport
        active_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/active', 'AGENCY_ID': ACTIVE_PARTY}
        active_components.mock_logger = MockLogger()
        task_info_a = SecureLGBMContext(args_a, active_components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label=True)
        secure_dataset_a = SecureDataset(task_info_a, model_data)
        booster_a = VerticalLGBMActiveParty(task_info_a, secure_dataset_a)
        print(secure_dataset_a.feature_name)
        print(secure_dataset_a.train_idx.shape)
        print(secure_dataset_a.train_X.shape)
        print(secure_dataset_a.train_y.shape)
        print(secure_dataset_a.test_idx.shape)
        print(secure_dataset_a.test_X.shape)
        print(secure_dataset_a.test_y.shape)

        passive_components = Initializer(log_config_path='', config_path='')
        passive_components.transport = self._passive_transport
        passive_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/passive', 'AGENCY_ID': PASSIVE_PARTY}
        passive_components.mock_logger = MockLogger()
        task_info_b = SecureLGBMContext(args_b, passive_components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label=False)
        secure_dataset_b = SecureDataset(task_info_b, model_data)
        booster_b = VerticalLGBMPassiveParty(task_info_b, secure_dataset_b)
        print(secure_dataset_b.feature_name)
        print(secure_dataset_b.train_idx.shape)
        print(secure_dataset_b.train_X.shape)
        print(secure_dataset_b.test_idx.shape)
        print(secure_dataset_b.test_X.shape)

        def active_worker():
            try:
                booster_a.fit()
                booster_a.save_model()
                train_praba = booster_a.get_train_praba()
                test_praba = booster_a.get_test_praba()
                Evaluation(task_info_a, secure_dataset_a,
                           train_praba, test_praba)
                # ModelPlot(booster_a)
                ResultFileHandling(task_info_a)
                booster_a.load_model()
                booster_a.predict()
                test_praba = booster_a.get_test_praba()
                task_info_a.algorithm_type = 'Predict'
                task_info_a.sync_file_list = {}
                Evaluation(task_info_a, secure_dataset_a,
                           test_praba=test_praba)
                ResultFileHandling(task_info_a)
            except Exception as e:
                task_info_a.components.logger().info(traceback.format_exc())

        def passive_worker():
            try:
                booster_b.fit()
                booster_b.save_model()
                train_praba = booster_b.get_train_praba()
                test_praba = booster_b.get_test_praba()
                Evaluation(task_info_b, secure_dataset_b,
                           train_praba, test_praba)
                # ModelPlot(booster_b)
                ResultFileHandling(task_info_b)
                booster_b.load_model()
                booster_b.predict()
                test_praba = booster_b.get_test_praba()
                task_info_b.algorithm_type = 'Predict'
                task_info_b.sync_file_list = {}
                Evaluation(task_info_b, secure_dataset_b,
                           test_praba=test_praba)
                ResultFileHandling(task_info_b)
            except Exception as e:
                task_info_b.components.logger().info(traceback.format_exc())

        thread_lgbm_active = threading.Thread(target=active_worker, args=())
        thread_lgbm_active.start()

        thread_lgbm_passive = threading.Thread(target=passive_worker, args=())
        thread_lgbm_passive.start()

        thread_lgbm_active.join()
        thread_lgbm_passive.join()


if __name__ == '__main__':
    unittest.main()
