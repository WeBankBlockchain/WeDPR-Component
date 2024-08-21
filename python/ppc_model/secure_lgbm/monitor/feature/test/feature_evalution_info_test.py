# -*- coding: utf-8 -*-
import unittest
import numpy as np
from ppc_model.secure_lgbm.monitor.feature.feature_evaluation_info import FeatureEvaluationResult
from ppc_model.secure_lgbm.monitor.feature.feature_evaluation_info import EvaluationMetric
from ppc_model.secure_lgbm.monitor.feature.feature_evaluation_info import EvaluationType


def check_result(ut_obj, evaluation_result, expected_ks, expected_auc, expected_sample):
    ut_obj.assertEqual(evaluation_result.ks.value,  expected_ks)
    ut_obj.assertEqual(evaluation_result.auc.value, expected_auc)
    ut_obj.assertEqual(evaluation_result.samples.value, expected_sample)


class TestFeatureEvaluationResult(unittest.TestCase):
    def test_default_table_meta(self):
        sample_num = 1000000
        train_label_list = np.random.randint(0, 2, sample_num)
        train_evaluation_result = FeatureEvaluationResult(
            type=EvaluationType.TRAIN, label_list=train_label_list)
        ks_value = 0.4126
        auc_value = 0.7685
        (train_evaluation_result.ks.value,
         train_evaluation_result.auc.value) = (ks_value, auc_value)
        check_result(self, train_evaluation_result,
                     ks_value, auc_value, sample_num)

        sample_num = 2000000
        validation_label_list = np.random.randint(0, 2, sample_num)
        validation_evaluation_result = FeatureEvaluationResult(
            type=EvaluationType.VALIDATION, label_list=validation_label_list)
        ks_value = 0.3116
        auc_value = 0.6676
        (validation_evaluation_result.ks.value,
         validation_evaluation_result.auc.value) = (ks_value, auc_value)
        check_result(self, validation_evaluation_result,
                     ks_value, auc_value, sample_num)

        local_path = "evaluation_result_case_1.csv"
        FeatureEvaluationResult.store_and_upload_summary(
            [train_evaluation_result, validation_evaluation_result], local_path, None, None)

    def test_with_given_table_meta(self):
        ks_value = 0.4126
        auc_value = 0.7685
        sample_num = 1000000
        train_label_list = np.random.randint(0, 2, sample_num)
        train_evaluation_result = FeatureEvaluationResult(
            type=EvaluationType.TRAIN, label_list=train_label_list, ks_value=ks_value, auc_value=auc_value)
        check_result(self, train_evaluation_result,
                     ks_value, auc_value, sample_num)
        train_evaluation_result.ks.desc = "KS值"
        train_evaluation_result.auc.desc = "AUC值"

        ks_value = 0.3116
        auc_value = 0.6676
        sample_num = 2000000
        validation_label_list = np.random.randint(0, 2, sample_num)
        validation_evaluation_result = FeatureEvaluationResult(
            type=EvaluationType.VALIDATION, label_list=validation_label_list, ks_value=ks_value, auc_value=auc_value)
        validation_evaluation_result.ks.desc = "KS值"
        validation_evaluation_result.auc.desc = "AUC值"
        check_result(self, validation_evaluation_result,
                     ks_value, auc_value, sample_num)

        local_path = "evaluation_result_case_2.csv"

        FeatureEvaluationResult.store_and_upload_summary(
            [train_evaluation_result, validation_evaluation_result], local_path, None, None)


if __name__ == '__main__':
    unittest.main()
