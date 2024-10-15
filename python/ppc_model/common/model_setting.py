# -*- coding: utf-8 -*-
from ppc_common.ppc_utils import common_func


class PreprocessingSetting:
    def __init__(self, model_dict):
        self.use_psi = common_func.get_config_value(
            "use_psi", False, model_dict, False)
        self.fillna = common_func.get_config_value(
            "fillna", False, model_dict, False)
        self.na_select = float(common_func.get_config_value(
            "na_select", 1.0, model_dict, False))
        self.filloutlier = common_func.get_config_value(
            "filloutlier", False, model_dict, False)
        self.normalized = common_func.get_config_value(
            "normalized", False, model_dict, False)
        self.standardized = common_func.get_config_value(
            "standardized", False, model_dict, False)
        self.categorical = common_func.get_config_value(
            "categorical", '', model_dict, False)
        self.psi_select_col = common_func.get_config_value(
            "psi_select_col", '', model_dict, False)
        self.psi_select_base = common_func.get_config_value(
            "psi_select_base", '', model_dict, False)
        self.psi_select_base = float(common_func.get_config_value(
            "psi_select_thresh", 0.3, model_dict, False))
        self.psi_select_bins = int(common_func.get_config_value(
            "psi_select_bins", 4, model_dict, False))
        self.corr_select = float(common_func.get_config_value(
            "corr_select", 0, model_dict, False))
        self.use_goss = common_func.get_config_value(
            "use_goss", False, model_dict, False)


class FeatureEngineeringEngineSetting:
    def __init__(self, model_dict):
        self.use_iv = common_func.get_config_value(
            "use_iv", False, model_dict, False)
        self.group_num = int(common_func.get_config_value(
            "group_num", 4, model_dict, False))
        self.iv_thresh = float(common_func.get_config_value(
            "iv_thresh", 0.1, model_dict, False))


class CommmonModelSetting:
    def __init__(self, model_dict):
        self.learning_rate = float(common_func.get_config_value(
            "learning_rate", 0.1, model_dict, False))

        self.eval_set_column = common_func.get_config_value(
            "eval_set_column", "", model_dict, False)
        self.train_set_value = common_func.get_config_value(
            "train_set_value", "", model_dict, False)
        self.eval_set_value = common_func.get_config_value(
            "eval_set_value", "", model_dict, False)
        self.verbose_eval = int(common_func.get_config_value(
            "verbose_eval", 1, model_dict, False))
        self.silent = common_func.get_config_value(
            "silent", False, model_dict, False)
        self.train_features = common_func.get_config_value(
            "train_features", "", model_dict, False)
        random_state_str = common_func.get_config_value(
            "random_state", "", model_dict, False)
        if len(random_state_str) > 0:
            self.random_state = int(random_state_str)
        self.n_jobs = int(common_func.get_config_value(
            "n_jobs", 0, model_dict, False))


class SecureLGBMSetting(CommmonModelSetting):
    def __init__(self, model_dict):
        super().__init__(model_dict)
        self.test_size = float(common_func.get_config_value(
            "test_dataset_percentage", 0.3, model_dict, False))
        self.num_trees = int(common_func.get_config_value(
            "num_trees", 6, model_dict, False))
        self.max_depth = int(common_func.get_config_value(
            "max_depth", 3, model_dict, False))
        self.max_bin = int(common_func.get_config_value(
            "max_bin", 4, model_dict, False))

        self.subsample = float(common_func.get_config_value(
            "subsample", 1, model_dict, False))
        self.colsample_bytree = float(common_func.get_config_value(
            "colsample_bytree", 1, model_dict, False))
        self.colsample_bylevel = float(common_func.get_config_value(
            "colsample_bylevel", 1, model_dict, False))
        self.reg_alpha = float(common_func.get_config_value(
            "reg_alpha", 0, model_dict, False))
        self.reg_lambda = float(common_func.get_config_value(
            "reg_lambda", 1, model_dict, False))
        self.gamma = float(common_func.get_config_value(
            "gamma", 0, model_dict, False))
        self.min_child_weight = float(common_func.get_config_value(
            "min_child_weight", 0.0, model_dict, False))
        self.min_child_samples = int(common_func.get_config_value(
            "min_child_samples", 10, model_dict, False))
        self.seed = int(common_func.get_config_value(
            "seed", 2024, model_dict, False))
        self.early_stopping_rounds = int(common_func.get_config_value(
            "early_stopping_rounds", 5, model_dict, False))
        self.eval_metric = common_func.get_config_value(
            "eval_metric", "auc", model_dict, False)
        self.threads = int(common_func.get_config_value(
            "threads", 8, model_dict, False))
        self.one_hot = common_func.get_config_value(
            "one_hot", 0, model_dict, False)


class SecureLRSetting(CommmonModelSetting):
    def __init__(self, model_dict):
        super().__init__(model_dict)
        self.feature_rate = float(common_func.get_config_value(
            "feature_rate", 1.0, model_dict, False))
        self.batch_size = int(common_func.get_config_value(
            "batch_size", 16, model_dict, False))
        self.epochs = int(common_func.get_config_value(
            "epochs", 3, model_dict, False))


class ModelSetting(PreprocessingSetting, FeatureEngineeringEngineSetting, SecureLGBMSetting, SecureLRSetting):
    def __init__(self, model_dict):
        # init PreprocessingSetting
        super().__init__(model_dict)
        # init FeatureEngineeringEngineSetting
        super(FeatureEngineeringEngineSetting, self).__init__(model_dict)
        # init SecureLGBMSetting
        super(SecureLGBMSetting, self).__init__(model_dict)
        # init SecureLRSetting
        super(SecureLRSetting, self).__init__(model_dict)
