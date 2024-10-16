import os
from enum import Enum
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_common.ppc_crypto.phe_factory import PheCipherFactory
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole
from ppc_common.ppc_utils import common_func
from ppc_model.common.model_setting import ModelSetting
from ppc_model.secure_model_base.secure_model_context import SecureModel
from ppc_model.secure_model_base.secure_model_context import SecureModelContext


class LRModel(SecureModel):

    def __init__(
        self,
        epochs: int = 10,
        batch_size: int = 8,
        learning_rate: float = 0.1,
        random_state: int = None,
        n_jobs: int = None,
        **kwargs
    ):

        self.epochs = epochs
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.random_state = random_state
        self.n_jobs = n_jobs
        super().__init__(**kwargs)


class ModelTaskParams(LRModel):
    def __init__(
        self,
        test_size: float = 0.3,
        feature_rate: float = 1.0,
        eval_set_column: str = None,
        train_set_value: str = None,
        eval_set_value: str = None,
        train_feats: str = None,
        verbose_eval: int = 1,
        categorical_feature: list = [],
        silent: bool = False
    ):

        super().__init__()

        self.test_size = test_size
        self.feature_rate = feature_rate
        self.eval_set_column = eval_set_column
        self.train_set_value = train_set_value
        self.eval_set_value = eval_set_value
        self.train_feature = train_feats
        self.verbose_eval = verbose_eval
        self.silent = silent
        self.lr = self.learning_rate
        self.categorical_feature = categorical_feature
        self.categorical_idx = []
        self.my_categorical_idx = []


class SecureLRParams(ModelTaskParams):

    def __init__(self):
        super().__init__()

    def _get_params(self):
        """返回LRClassifier所有参数"""
        return LRModel().get_params()


class SecureLRContext(SecureModelContext):

    def __init__(self,
                 args,
                 components: Initializer
                 ):
        super().__init__(args, components)

        self.phe = PheCipherFactory.build_phe(
            components.homo_algorithm, components.public_key_length)
        self.codec = PheCipherFactory.build_codec(components.homo_algorithm)

    def create_model_param(self):
        return SecureLRParams()

    def get_model_params(self):
        """获取lr参数"""
        return self.model_params

    def set_sync_file(self):
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


class LRMessage(Enum):
    FEATURE_NAME = "FEATURE_NAME"
    ENC_D_LIST = "ENC_D_LIST"
    ENC_D_HIST = "ENC_D_HIST"
    D_MATMUL = "D_MATMUL"
    PREDICT_LEAF_MASK = "PREDICT_LEAF_MASK"
    TEST_LEAF_MASK = "PREDICT_TEST_LEAF_MASK"
    VALID_LEAF_MASK = "PREDICT_VALID_LEAF_MASK"
    PREDICT_PRABA = "PREDICT_PRABA"
    MODEL_DATA = "MODEL_DATA"
