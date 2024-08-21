# -*- coding: utf-8 -*-
import unittest
from ppc_common.ppc_ml.feature.feature_importance import FeatureImportanceType
from ppc_common.ppc_ml.feature.feature_importance import FeatureImportanceStore
from ppc_common.ppc_ml.feature.feature_importance import ReadOnlyFeatureImportanceStore
from ppc_common.deps_services.serialize_type import SerializeType
import logging
import random
import sys
import pandas as pd


class FeatureImportanceWrapper:
    def __init__(self, ut_obj, feature_size):
        self.ut_obj = ut_obj
        self.feature_size = feature_size
        self.feature_list = []
        self._fake_features()
        logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
        self.logger = logging.getLogger(__name__)
        self.feature_importance_type_list = [
            FeatureImportanceType.GAIN, FeatureImportanceType.WEIGHT]
        self.feature_importance_store = FeatureImportanceStore(
            self.feature_importance_type_list, self.feature_list, self.logger)
        self.epsilon = 0.000000001

    def _fake_features(self):
        for i in range(self.feature_size):
            self.feature_list.append("feature_" + str(i))

    def update_feature_importance_and_check(self, rounds, local_file_path):
        for i in range(rounds):
            selected_feature = random.randint(0, self.feature_size - 1)
            key = self.feature_importance_store.generate_fid_key(
                selected_feature)
            gain = random.uniform(0, 101)
            pre_gain = self.feature_importance_store.get_feature_importance(
                key, FeatureImportanceType.GAIN)
            pre_weight = self.feature_importance_store.get_feature_importance(
                key, FeatureImportanceType.WEIGHT)
            self.feature_importance_store.update_feature_importance(
                selected_feature, {FeatureImportanceType.GAIN: gain, FeatureImportanceType.WEIGHT: 1})
            self.ut_obj.assertEqual(
                pre_gain + gain, self.feature_importance_store.get_feature_importance(key, FeatureImportanceType.GAIN))
            self.ut_obj.assertEqual(
                pre_weight + 1, self.feature_importance_store.get_feature_importance(key, FeatureImportanceType.WEIGHT))
        # store
        self.feature_importance_store.store(
            SerializeType.CSV, local_file_path, None, None)
        # load
        df = pd.read_csv(local_file_path)
        loaded_feature_importance_store = ReadOnlyFeatureImportanceStore.load(
            df, self.logger)
        # check the dict
        for importance_type in loaded_feature_importance_store.feature_importance_dict:
            self.ut_obj.assertTrue(
                importance_type in self.feature_importance_store.feature_importance_dict)
            feature_importances = self.feature_importance_store.feature_importance_dict[
                importance_type]
            loaded_feature_importances = loaded_feature_importance_store.feature_importance_dict[
                importance_type]
            sum_data = 0
            for fid in loaded_feature_importances.keys():
                sum_data += loaded_feature_importances[fid].importance
            self.ut_obj.assertTrue(abs(sum_data - 1) < self.epsilon)
        local_json_file = local_file_path + ".json"
        self.feature_importance_store.store(
            SerializeType.JSON, local_json_file, None, None)


class TestFeatureImportance(unittest.TestCase):
    def test_update_gain_and_store(self):
        feature_size = 100
        self.wrapper = FeatureImportanceWrapper(
            self, feature_size=feature_size)
        rounds = 100000
        local_file_path = "feature_importance_case1.csv"
        self.wrapper.update_feature_importance_and_check(
            rounds, local_file_path=local_file_path)
