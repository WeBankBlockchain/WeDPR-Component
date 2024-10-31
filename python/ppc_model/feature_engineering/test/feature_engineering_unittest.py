import multiprocessing
import threading
import traceback
import unittest

import numpy as np

from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_common.ppc_mock.mock_objects import MockLogger, MockStorageClient
from ppc_model.common.initializer import Initializer
from ppc_model.common.mock.mock_model_transport import MockModelRouterApi
from ppc_model.common.protocol import TaskRole
from ppc_model.feature_engineering.feature_engineering_context import FeatureEngineeringContext
from ppc_model.feature_engineering.vertical.active_party import VerticalFeatureEngineeringActiveParty
from ppc_model.feature_engineering.vertical.passive_party import VerticalFeatureEngineeringPassiveParty

ACTIVE_PARTY = 'ACTIVE_PARTY'

PASSIVE_PARTY = 'PASSIVE_PARTY'


def construct_dataset(num_samples, num_features):
    np.random.seed(0)
    # 生成标签列
    labels = np.random.choice([0, 1], size=num_samples)
    # 生成特征列
    features = np.random.rand(num_samples, num_features)
    return labels, features


def mock_args(num_features, iv_thresh):
    job_id = '0x12345678'
    active_fields = ['a' + str(i) for i in range(num_features)]
    passive_fields = ['b' + str(i) for i in range(num_features)]

    model_config_dict = {
        'use_iv': True,
        'iv_thresh': iv_thresh,
        'categorical': '0',
        'group_num': 100,

    }

    args_a = {
        'job_id': job_id,
        'task_id': job_id,
        'feature_name_list': active_fields,
        'participant_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'result_receiver_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'model_dict': model_config_dict,
    }

    args_b = {
        'job_id': job_id,
        'task_id': job_id,
        'feature_name_list': passive_fields,
        'participant_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'result_receiver_id_list': [ACTIVE_PARTY, PASSIVE_PARTY],
        'model_dict': model_config_dict,
    }
    return args_a, args_b


class TestFeatureEngineering(unittest.TestCase):

    def setUp(self):
        participants = [PASSIVE_PARTY, ACTIVE_PARTY]
        self._active_transport = MockModelRouterApi(participants)
        self._passive_transport = MockModelRouterApi(participants)

    def test_fit(self):
        num_samples = 100000
        num_features = 100
        iv_thresh = 0.05
        labels, features = construct_dataset(num_samples, num_features)
        args_a, args_b = mock_args(num_features, iv_thresh)

        active_components = Initializer(log_config_path='', config_path='')
        active_components.homo_algorithm = 0
        active_components.transport = self._active_transport
        active_components.config_data = {'JOB_TEMP_DIR': '/tmp'}
        active_components.mock_logger = MockLogger()
        active_components.storage_client = MockStorageClient()
        active_context = FeatureEngineeringContext(
            task_id=args_a['task_id'],
            args=args_a,
            components=active_components,
            role=TaskRole.ACTIVE_PARTY,
            feature=features,
            feature_name_list=args_a['feature_name_list'],
            label=labels
        )
        active_vfe = VerticalFeatureEngineeringActiveParty(active_context)

        passive_components = Initializer(log_config_path='', config_path='')
        passive_components.homo_algorithm = 0
        passive_components.transport = self._passive_transport
        passive_components.config_data = {'JOB_TEMP_DIR': '/tmp'}
        passive_components.mock_logger = MockLogger()
        passive_components.storage_client = MockStorageClient()
        passive_context = FeatureEngineeringContext(
            task_id=args_b['task_id'],
            args=args_b,
            components=passive_components,
            role=TaskRole.PASSIVE_PARTY,
            feature=features,
            feature_name_list=args_b['feature_name_list'],
            label=None
        )
        passive_vfe = VerticalFeatureEngineeringPassiveParty(passive_context)

        def active_worker():
            try:
                active_vfe.fit()
            except Exception as e:
                active_components.logger().info(traceback.format_exc())

        def passive_worker():
            try:
                passive_vfe.fit()
            except Exception as e:
                active_components.logger().info(traceback.format_exc())

        thread_fe_active = threading.Thread(target=active_worker, args=())
        thread_fe_active.start()

        thread_fe_passive = threading.Thread(target=passive_worker, args=())
        thread_fe_passive.start()

        thread_fe_active.join()
        thread_fe_passive.join()


if __name__ == '__main__':
    multiprocessing.set_start_method('spawn')
    unittest.main()
