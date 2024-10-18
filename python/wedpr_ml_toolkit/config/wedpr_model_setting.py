# -*- coding: utf-8 -*-

class PreprocessingModelSetting:
    def __init__(self):
        self.use_psi = False
        self.fillna = False
        self.na_select = 1.0
        self.filloutlier = False
        self.normalized = False
        self.standardized = False
        self.categorical = ''
        self.psi_select_col = ''
        self.psi_select_base = ''
        self.psi_select_base = 0.3
        self.psi_select_bins = 4
        self.corr_select = 0
        self.use_goss = False


class FeatureEngineeringEngineModelSetting:
    def __init__(self):
        self.use_iv = False
        self.group_num = 4
        self.iv_thresh = 0.1


class CommmonSecureModelSetting:
    def __init__(self):
        self.learning_rate = 0.1
        self.eval_set_column = ''
        self.train_set_value = ''
        self.eval_set_value = ''
        self.verbose_eval = 1
        self.silent = False
        self.train_features = ''
        self.random_state = None
        self.n_jobs = 0


class SecureLGBMModelSetting(CommmonSecureModelSetting):
    def __init__(self):
        super().__init__()
        self.test_size = 0.3
        self.num_trees = 6
        self.max_depth = 3
        self.max_bin = 4
        self.subsample = 1.0
        self.colsample_bytree = 1
        self.colsample_bylevel = 1
        self.reg_alpha = 0
        self.reg_lambda = 1.0
        self.gamma = 0.0
        self.min_child_weight = 0.0
        self.min_child_samples = 10
        self.seed = 2024
        self.early_stopping_rounds = 5
        self.eval_metric = "auc"
        self.threads = 8
        self.one_hot = 0


class SecureLRModelSetting(CommmonSecureModelSetting):
    def __init__(self):
        super().__init__()
        self.feature_rate = 1.0
        self.batch_size = 16
        self.epochs = 3


class ModelSetting(PreprocessingModelSetting, FeatureEngineeringEngineModelSetting, SecureLGBMModelSetting, SecureLRModelSetting):
    def __init__(self):
        # init PreprocessingSetting
        super().__init__()
        # init FeatureEngineeringEngineSetting
        super(FeatureEngineeringEngineModelSetting, self).__init__(model_dict)
        # init SecureLGBMSetting
        super(SecureLGBMModelSetting, self).__init__(model_dict)
        # init SecureLRSetting
        super(SecureLRModelSetting, self).__init__(model_dict)
