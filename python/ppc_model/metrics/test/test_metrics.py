import os
import unittest
import numpy as np

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.metrics.evaluation import Evaluation
from ppc_model.metrics.model_plot import ModelPlot, DiGraphTree
from ppc_model.common.model_result import ResultFileHandling
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
        'max_bin': 10,
        'n_estimators': 2,
        'max_depth': 3,
        'use_goss': 1,
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


class TestXgboostTraining(unittest.TestCase):

    args_a, args_b = mock_args()

    def test_active_metrics(self):

        active_components = Initializer(log_config_path='', config_path='')
        active_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/active', 'AGENCY_ID': ACTIVE_PARTY}
        active_components.mock_logger = MockLogger()
        task_info_a = SecureLGBMContext(self.args_a['task_id'], self.args_a, active_components)
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

        # booster_a._train_praba = np.random.rand(len(secure_dataset_a.train_y))
        booster_a._test_praba = np.random.rand(len(secure_dataset_a.test_y))

        Evaluation(task_info_a, secure_dataset_a,
                   booster_a._train_praba, booster_a._test_praba)

    def test_passive_metrics(self):

        passive_components = Initializer(log_config_path='', config_path='')
        passive_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/passive', 'AGENCY_ID': PASSIVE_PARTY}
        passive_components.mock_logger = MockLogger()
        task_info_b = SecureLGBMContext(self.args_b['task_id'], self.args_b, passive_components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label=False)
        secure_dataset_b = SecureDataset(task_info_b, model_data)
        booster_b = VerticalLGBMPassiveParty(task_info_b, secure_dataset_b)
        print(secure_dataset_b.feature_name)
        print(secure_dataset_b.train_idx.shape)
        print(secure_dataset_b.train_X.shape)
        print(secure_dataset_b.test_idx.shape)
        print(secure_dataset_b.test_X.shape)

        # booster_b._train_praba = np.random.rand(len(secure_dataset_b.train_idx))
        booster_b._test_praba = np.random.rand(len(secure_dataset_b.test_idx))

        Evaluation(task_info_b, secure_dataset_b,
                   booster_b._train_praba, booster_b._test_praba)

    def test_model_plot(self):

        active_components = Initializer(log_config_path='', config_path='')
        active_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/active', 'AGENCY_ID': ACTIVE_PARTY}
        active_components.mock_logger = MockLogger()
        task_info_a = SecureLGBMContext(self.args_a['task_id'], self.args_a, active_components)
        model_data = SecureDataset.simulate_dataset(
            data_size, feature_dim, has_label=True)
        secure_dataset_a = SecureDataset(task_info_a, model_data)
        booster_a = VerticalLGBMActiveParty(task_info_a, secure_dataset_a)
        if os.path.exists(booster_a.ctx.model_data_file):
            booster_a.load_model()
            ModelPlot(booster_a)

    def test_digraphtree(self):
        Gtree = DiGraphTree()
        Gtree.add_node(0)
        Gtree.add_nodes_from([1, 2])
        Gtree.add_weighted_edges_from(
            [(0, 1, 'left_'+str(2)+'_'+str(3)+'_'+str(0.5)),
             (0, 2, 'right_'+str(2)+'_'+str(3)+'_'+str(0.9))])
        Gtree.add_nodes_from([3, 4])
        Gtree.add_weighted_edges_from(
            [(1, 3, 'left_'+str(20)+'_'+str(4)+'_'+str(0.5)),
             (1, 4, 'right_'+str(20)+'_'+str(4)+'_'+str(0.9))])
        Gtree.add_nodes_from([5, 6])
        Gtree.add_weighted_edges_from(
            [(2, 5, 'left_'+str(2)+'_'+str(7)+'_'+str(0.5)),
             (2, 6, 'right_'+str(2)+'_'+str(7)+'_'+str(0.9))])
        Gtree.add_nodes_from([7, 8])
        Gtree.add_weighted_edges_from(
            [(3, 7, 'left_'+str(1)+'_'+str(11)+'_'+str(0.5)),
             (3, 8, 'right_'+str(1)+'_'+str(11)+'_'+str(0.9))])
        Gtree.add_nodes_from([9, 10])
        Gtree.add_weighted_edges_from(
            [(4, 9, 'left_'+str(18)+'_'+str(2)+'_'+str(0.5)),
             (4, 10, 'right_'+str(18)+'_'+str(2)+'_'+str(0.9))])
        Gtree.add_nodes_from([11, 12])
        Gtree.add_weighted_edges_from(
            [(5, 11, 'left_'+str(23)+'_'+str(25)+'_'+str(0.5)),
             (5, 12, 'right_'+str(23)+'_'+str(25)+'_'+str(0.9))])
        Gtree.add_nodes_from([13, 14])
        Gtree.add_weighted_edges_from(
            [(6, 13, 'left_'+str(16)+'_'+str(10)+'_'+str(0.5)),
             (6, 14, 'right_'+str(16)+'_'+str(10)+'_'+str(0.9))])

        # Gtree.tree_plot()
        # Gtree.tree_plot(split=False, figsize=(10, 5))
        # Gtree.tree_plot(figsize=(6, 3))
        Gtree.tree_plot(figsize=(10, 5), save_filename='tree.svg')


if __name__ == '__main__':
    unittest.main()
