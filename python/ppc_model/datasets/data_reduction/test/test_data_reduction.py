import unittest
import numpy as np

from ppc_model.datasets.data_reduction.feature_selection import FeatureSelection
from ppc_model.datasets.data_reduction.sampling import Sampling


class TestFeatureSelection(unittest.TestCase):

    feature_name = [f'x{i+1}' for i in range(30)]

    def test_fr_feature_select(self):

        feature_select = FeatureSelection.feature_selecting(
            self.feature_name, [], 0.8)
        self.assertEqual(len(feature_select), len(self.feature_name) * 0.8)

    def test_customized_feature_select(self):

        train_feats = ['x1', 'x3', 'x15', 'x27', 'x33']
        feature_select = FeatureSelection.feature_selecting(
            self.feature_name, train_feats, 0.8)
        self.assertEqual(len(feature_select), len(
            set(self.feature_name).intersection(set(train_feats))))
        self.assertEqual(sorted([f'x{i+1}' for i in feature_select]),
                         sorted(set(self.feature_name).intersection(set(train_feats))))

    def test_feature_select(self):

        feature_select = FeatureSelection.feature_selecting(
            self.feature_name, [], 0)
        self.assertEqual(len(feature_select), len(self.feature_name))
        self.assertEqual(feature_select, list(range(len(self.feature_name))))


class TestSampling(unittest.TestCase):

    g_list = [np.random.rand() for i in range(500)]
    h_list = [np.random.rand() for i in range(500)]

    def test_goss_sampling(self):
        instance, used_glist, used_hlist = Sampling.sample_selecting(
            self.g_list, self.h_list, use_goss=True)
        self.assertEqual(len(instance), int(
            len(self.g_list) * 0.2) + int(len(self.g_list) * 0.1))
        assert max(self.g_list) in used_glist
        assert np.argmax(self.g_list) in instance

    def test_subsample_sampling(self):
        instance, used_glist, used_hlist = Sampling.sample_selecting(
            self.g_list, self.h_list, subsample=0.6)
        self.assertEqual(len(instance), int(len(self.g_list) * 0.6))

    def test_sampling(self):
        instance, used_glist, used_hlist = Sampling.sample_selecting(
            self.g_list, self.h_list)
        self.assertEqual(len(instance), len(self.g_list))


if __name__ == "__main__":
    unittest.main()
