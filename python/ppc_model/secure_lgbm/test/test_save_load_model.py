import unittest
import numpy as np

from ppc_model.common.initializer import Initializer
from ppc_common.ppc_mock.mock_objects import MockLogger
from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext
from ppc_model.secure_lgbm.vertical.booster import VerticalBooster


class TestSaveLoadModel(unittest.TestCase):

    n_estimators = 2
    max_depth = 3
    np.random.seed(2024)

    ACTIVE_PARTY = 'ACTIVE_PARTY'
    PASSIVE_PARTY = 'PASSIVE_PARTY'

    job_id = 'j-123'
    task_id = 't-123'

    model_dict = {}

    args = {
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

    components = Initializer(log_config_path='', config_path='')
    components.config_data = {'JOB_TEMP_DIR': '/tmp'}
    components.mock_logger = MockLogger()

    def test_save_load_model(self):

        x_split = [[1, 2, 3, 5], [1, 2], [1.23, 3.45, 5.23]]

        trees = []
        for i in range(self.n_estimators):
            tree = self._build_tree(self.max_depth)
            trees.append(tree)
        # print(trees)

        task_info = SecureLGBMContext(self.args['task_id'], self.args, self.components)
        booster = VerticalBooster(task_info, dataset=None)
        booster._X_split = x_split
        booster._trees = trees
        booster.save_model()

        booster_predict = VerticalBooster(task_info, dataset=None)
        booster_predict.load_model()

        assert x_split == booster_predict._X_split
        assert trees == booster_predict._trees

    @staticmethod
    def _build_tree(max_depth, depth=0, weight=0):

        if depth == max_depth:
            return weight

        best_split_info = BestSplitInfo(
            feature=np.random.randint(0, 10),
            value=np.random.randint(0, 4),
            best_gain=np.random.rand(),
            w_left=np.random.rand(),
            w_right=np.random.rand(),
            agency_idx=np.random.randint(0, 2),
            agency_feature=np.random.randint(0, 5)
        )
        # print(best_split_info)

        if best_split_info.best_gain > 0.2:
            left_tree = TestSaveLoadModel._build_tree(
                max_depth, depth + 1, best_split_info.w_left)
            right_tree = TestSaveLoadModel._build_tree(
                max_depth, depth + 1, best_split_info.w_right)

            return [(best_split_info, left_tree, right_tree)]
        else:
            return weight


if __name__ == '__main__':
    unittest.main()
