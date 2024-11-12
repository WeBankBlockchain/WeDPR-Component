
import os
from enum import Enum
from sklearn.base import BaseEstimator

from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_common.ppc_crypto.phe_factory import PheCipherFactory
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole
from ppc_common.ppc_utils import common_func
from ppc_model.common.model_setting import ModelSetting
from ppc_model.secure_model_base.secure_model_context import SecureModel
from ppc_model.secure_model_base.secure_model_context import SecureModelContext


class LGBMModel(SecureModel):

    def __init__(
        self,
        boosting_type: str = 'gbdt',
        num_leaves: int = 31,
        max_depth: int = -1,
        learning_rate: float = 0.1,
        n_estimators: int = 100,
        subsample_for_bin: int = 200000,
        objective: str = None,
        min_split_gain: float = 0.,
        min_child_weight: float = 1e-3,
        min_child_samples: int = 20,
        subsample: float = 1.,
        subsample_freq: int = 0,
        colsample_bytree: float = 1.,
        reg_alpha: float = 0.,
        reg_lambda: float = 0.,
        random_state: int = None,
        n_jobs: int = None,
        importance_type: str = 'split',
        **kwargs
    ):
        self.boosting_type = boosting_type
        self.objective = objective
        self.num_leaves = num_leaves
        self.max_depth = max_depth
        self.learning_rate = learning_rate
        self.n_estimators = n_estimators
        self.subsample_for_bin = subsample_for_bin
        self.min_split_gain = min_split_gain
        self.min_child_weight = min_child_weight
        self.min_child_samples = min_child_samples
        self.subsample = subsample
        self.subsample_freq = subsample_freq
        self.colsample_bytree = colsample_bytree
        self.reg_alpha = reg_alpha
        self.reg_lambda = reg_lambda
        self.random_state = random_state
        self.n_jobs = n_jobs
        self.importance_type = importance_type
        super().__init__(**kwargs)


class ModelTaskParams(LGBMModel):
    def __init__(
        self,
        test_size: float = 0.3,
        max_bin: int = 10,
        use_goss: bool = False,
        top_rate: float = 0.2,
        other_rate: float = 0.1,
        feature_rate: float = 1.0,
        colsample_bylevel: float = 1.0,
        gamma: float = 0,
        loss_type: str = 'logistic',
        eval_set_column: str = None,
        train_set_value: str = None,
        eval_set_value: str = None,
        train_feats: str = None,
        early_stopping_rounds: int = 5,
        eval_metric: str = 'auc',
        verbose_eval: int = 1,
        categorical_feature: list = [],
        silent: bool = False
    ):

        super().__init__()

        self.test_size = test_size
        self.max_bin = max_bin
        self.use_goss = use_goss
        self.top_rate = top_rate
        self.other_rate = other_rate
        self.feature_rate = feature_rate
        self.colsample_bylevel = colsample_bylevel
        self.gamma = gamma
        self.loss_type = loss_type
        self.eval_set_column = eval_set_column
        self.train_set_value = train_set_value
        self.eval_set_value = eval_set_value
        self.train_feature = train_feats
        self.early_stopping_rounds = early_stopping_rounds
        self.eval_metric = eval_metric
        self.verbose_eval = verbose_eval
        self.silent = silent
        self.λ = self.reg_lambda
        self.lr = self.learning_rate
        self.categorical_feature = categorical_feature
        self.categorical_idx = []
        self.my_categorical_idx = []


class SecureLGBMParams(ModelTaskParams):

    def __init__(self):
        super().__init__()

    def _get_params(self):
        """返回LGBMClassifier所有参数"""
        return LGBMModel().get_params()


class SecureLGBMContext(SecureModelContext):

    def __init__(self,
                 task_id,
                 args,
                 components: Initializer
                 ):
        super().__init__(task_id, args, components)

        self.phe = PheCipherFactory.build_phe(
            components.homo_algorithm, components.public_key_length)
        self.codec = PheCipherFactory.build_codec(components.homo_algorithm)

    def create_model_param(self):
        return SecureLGBMParams()

    def get_model_params(self):
        """获取lgbm参数"""
        return self.model_params

    def set_sync_file(self):
        self.sync_file_list['metrics_iteration'] = [
            self.metrics_iteration_file, self.remote_metrics_iteration_file]
        self.sync_file_list['feature_importance'] = [
            self.feature_importance_file, self.remote_feature_importance_file]
        self.sync_file_list['summary_evaluation'] = [
            self.summary_evaluation_file, self.remote_summary_evaluation_file]
        self.sync_file_list['train_ks_table'] = [
            self.train_metric_ks_table, self.remote_train_metric_ks_table]
        self.sync_file_list['train_metric_roc'] = [
            self.train_metric_roc_file, self.remote_train_metric_roc_file]
        self.sync_file_list['train_metric_ks'] = [
            self.train_metric_ks_file, self.remote_train_metric_ks_file]
        self.sync_file_list['train_metric_pr'] = [
            self.train_metric_pr_file, self.remote_train_metric_pr_file]
        self.sync_file_list['train_metric_acc'] = [
            self.train_metric_acc_file, self.remote_train_metric_acc_file]
        self.sync_file_list['test_ks_table'] = [
            self.test_metric_ks_table, self.remote_test_metric_ks_table]
        self.sync_file_list['test_metric_roc'] = [
            self.test_metric_roc_file, self.remote_test_metric_roc_file]
        self.sync_file_list['test_metric_ks'] = [
            self.test_metric_ks_file, self.remote_test_metric_ks_file]
        self.sync_file_list['test_metric_pr'] = [
            self.test_metric_pr_file, self.remote_test_metric_pr_file]
        self.sync_file_list['test_metric_acc'] = [
            self.test_metric_acc_file, self.remote_test_metric_acc_file]


class LGBMMessage(Enum):
    FEATURE_NAME = "FEATURE_NAME"
    INSTANCE = "INSTANCE"
    ENC_GH_LIST = "ENC_GH_LIST"
    ENC_GH_HIST = "ENC_GH_HIST"
    SPLIT_INFO = 'SPLIT_INFO'
    INSTANCE_MASK = "INSTANCE_MASK"
    PREDICT_LEAF_MASK = "PREDICT_LEAF_MASK"
    TEST_LEAF_MASK = "PREDICT_TEST_LEAF_MASK"
    VALID_LEAF_MASK = "PREDICT_VALID_LEAF_MASK"
    STOP_ITERATION = "STOP_ITERATION"
    PREDICT_PRABA = "PREDICT_PRABA"
    MODEL_DATA = "MODEL_DATA"
