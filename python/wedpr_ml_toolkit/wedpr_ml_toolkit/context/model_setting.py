# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.common.utils import utils
from wedpr_ml_toolkit.common.utils.base_object import BaseObject


class PreprocessingSetting(BaseObject):
    def __init__(self, model_dict=None):
        self.use_psi = utils.get_config_value(
            "use_psi", False, model_dict, False)
        self.fillna = utils.get_config_value(
            "fillna", False, model_dict, False)
        self.na_select = float(utils.get_config_value(
            "na_select", 1.0, model_dict, False))
        self.filloutlier = utils.get_config_value(
            "filloutlier", False, model_dict, False)
        self.normalized = utils.get_config_value(
            "normalized", False, model_dict, False)
        self.standardized = utils.get_config_value(
            "standardized", False, model_dict, False)
        self.categorical = utils.get_config_value(
            "categorical", '', model_dict, False)
        self.psi_select_col = utils.get_config_value(
            "psi_select_col", '', model_dict, False)
        self.psi_select_base = utils.get_config_value(
            "psi_select_base", '', model_dict, False)
        self.psi_select_base = float(utils.get_config_value(
            "psi_select_thresh", 0.3, model_dict, False))
        self.psi_select_bins = int(utils.get_config_value(
            "psi_select_bins", 4, model_dict, False))
        self.corr_select = float(utils.get_config_value(
            "corr_select", 0, model_dict, False))
        self.use_goss = utils.get_config_value(
            "use_goss", False, model_dict, False)


class FeatureEngineeringEngineSetting(BaseObject):
    def __init__(self, model_dict=None):
        self.use_iv = utils.get_config_value(
            "use_iv", False, model_dict, False)
        self.group_num = int(utils.get_config_value(
            "group_num", 4, model_dict, False))
        self.iv_thresh = float(utils.get_config_value(
            "iv_thresh", 0.1, model_dict, False))


class CommonModelSetting(BaseObject):
    def __init__(self, model_dict=None):
        self.learning_rate = float(utils.get_config_value(
            "learning_rate", 0.1, model_dict, False))

        self.eval_set_column = utils.get_config_value(
            "eval_set_column", "", model_dict, False)
        self.train_set_value = utils.get_config_value(
            "train_set_value", "", model_dict, False)
        self.eval_set_value = utils.get_config_value(
            "eval_set_value", "", model_dict, False)
        self.verbose_eval = int(utils.get_config_value(
            "verbose_eval", 1, model_dict, False))
        self.silent = utils.get_config_value(
            "silent", False, model_dict, False)
        self.train_features = utils.get_config_value(
            "train_features", "", model_dict, False)
        random_state_str = utils.get_config_value(
            "random_state", "", model_dict, False)
        if len(random_state_str) > 0:
            self.random_state = int(random_state_str)
        self.n_jobs = int(utils.get_config_value(
            "n_jobs", 0, model_dict, False))


class SecureLGBMSetting(CommonModelSetting, BaseObject):
    def __init__(self, model_dict=None):
        super().__init__(model_dict)
        self.test_size = float(utils.get_config_value(
            "test_dataset_percentage", 0.3, model_dict, False))
        self.num_trees = int(utils.get_config_value(
            "num_trees", 6, model_dict, False))
        self.max_depth = int(utils.get_config_value(
            "max_depth", 3, model_dict, False))
        self.max_bin = int(utils.get_config_value(
            "max_bin", 4, model_dict, False))

        self.subsample = float(utils.get_config_value(
            "subsample", 1, model_dict, False))
        self.colsample_bytree = float(utils.get_config_value(
            "colsample_bytree", 1, model_dict, False))
        self.colsample_bylevel = float(utils.get_config_value(
            "colsample_bylevel", 1, model_dict, False))
        self.reg_alpha = float(utils.get_config_value(
            "reg_alpha", 0, model_dict, False))
        self.reg_lambda = float(utils.get_config_value(
            "reg_lambda", 1, model_dict, False))
        self.gamma = float(utils.get_config_value(
            "gamma", 0, model_dict, False))
        self.min_child_weight = float(utils.get_config_value(
            "min_child_weight", 0.0, model_dict, False))
        self.min_child_samples = int(utils.get_config_value(
            "min_child_samples", 10, model_dict, False))
        self.seed = int(utils.get_config_value(
            "seed", 2024, model_dict, False))
        self.early_stopping_rounds = int(utils.get_config_value(
            "early_stopping_rounds", 5, model_dict, False))
        self.eval_metric = utils.get_config_value(
            "eval_metric", "auc", model_dict, False)
        self.threads = int(utils.get_config_value(
            "threads", 8, model_dict, False))
        self.one_hot = utils.get_config_value(
            "one_hot", 0, model_dict, False)


class SecureLRSetting(CommonModelSetting, BaseObject):
    def __init__(self, model_dict=None):
        super().__init__(model_dict)
        self.feature_rate = float(utils.get_config_value(
            "feature_rate", 1.0, model_dict, False))
        self.batch_size = int(utils.get_config_value(
            "batch_size", 16, model_dict, False))
        self.epochs = int(utils.get_config_value(
            "epochs", 3, model_dict, False))


class ModelSetting(PreprocessingSetting, FeatureEngineeringEngineSetting, SecureLGBMSetting, SecureLRSetting, BaseObject):
    def __init__(self, model_dict=None):
        # init PreprocessingSetting
        super().__init__(model_dict)
        # init FeatureEngineeringEngineSetting
        FeatureEngineeringEngineSetting.__init__(self, model_dict)
        # init SecureLGBMSetting
        SecureLGBMSetting.__init__(self, model_dict)
        # init SecureLRSetting
        SecureLRSetting.__init__(self, model_dict)
