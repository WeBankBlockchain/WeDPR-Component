import os
from enum import Enum
from typing import Any, Dict
from sklearn.base import BaseEstimator

from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_common.ppc_crypto.phe_factory import PheCipherFactory
from ppc_model.common.context import Context
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole
from ppc_common.ppc_utils import common_func
from ppc_model.common.model_setting import ModelSetting


class LRModel(BaseEstimator):

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
        self._other_params: Dict[str, Any] = {}
        self.set_params(**kwargs)

    def get_params(self, deep: bool = True) -> Dict[str, Any]:
        """Get parameters for this estimator.

        Parameters
        ----------
        deep : bool, optional (default=True)
            If True, will return the parameters for this estimator and
            contained subobjects that are estimators.

        Returns
        -------
        params : dict
            Parameter names mapped to their values.
        """
        params = super().get_params(deep=deep)
        params.update(self._other_params)
        return params

    def set_model_setting(self, model_setting: ModelSetting) -> "LRModel":
        # 获取对象的所有属性名
        attrs = dir(model_setting)
        # 过滤掉以_或者__开头的属性（这些通常是特殊方法或内部属性）
        attrs = [attr for attr in attrs if not attr.startswith('_')]

        params = {}
        for attr in attrs:
            try:
                setattr(self, attr, getattr(model_setting, attr))
            except Exception as e:
                pass
        return self

    def set_params(self, **params: Any) -> "LRModel":
        """Set the parameters of this estimator.

        Parameters
        ----------
        **params
            Parameter names with their new values.

        Returns
        -------
        self : object
            Returns self.
        """
        for key, value in params.items():
            setattr(self, key, value)
            if hasattr(self, f"_{key}"):
                setattr(self, f"_{key}", value)
            self._other_params[key] = value
        return self


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

    def get_all_params(self):
        """返回SecureLRParams所有参数"""
        # 获取对象的所有属性名
        attrs = dir(self)
        # 过滤掉以_或者__开头的属性（这些通常是特殊方法或内部属性）
        attrs = [attr for attr in attrs if not attr.startswith('_')]

        params = {}
        for attr in attrs:
            try:
                # 使用getattr来获取属性的值
                value = getattr(self, attr)
                # 检查value是否可调用（例如，方法或函数），如果是，则不打印其值
                if not callable(value):
                    params[attr] = value
            except Exception as e:
                pass
        return params


class SecureLRContext(Context):

    def __init__(self,
                 args,
                 components: Initializer
                 ):

        if args['is_label_holder']:
            role = TaskRole.ACTIVE_PARTY
        else:
            role = TaskRole.PASSIVE_PARTY

        super().__init__(args['job_id'],
                         args['task_id'],
                         components,
                         role)

        self.phe = PheCipherFactory.build_phe(
            components.homo_algorithm, components.public_key_length)
        self.codec = PheCipherFactory.build_codec(components.homo_algorithm)
        self.is_label_holder = args['is_label_holder']
        self.result_receiver_id_list = args['result_receiver_id_list']
        self.participant_id_list = args['participant_id_list']
        self.model_predict_algorithm = common_func.get_config_value(
            "model_predict_algorithm", None, args, False)
        self.algorithm_type = args['algorithm_type']
        if 'dataset_id' in args and args['dataset_id'] is not None:
            self.dataset_file_path = os.path.join(
                self.workspace, args['dataset_id'])
        else:
            self.dataset_file_path = None

        self.model_params = SecureLRParams()
        model_setting = ModelSetting(args['model_dict'])
        self.set_model_params(model_setting)
        if model_setting.train_features is not None and len(model_setting.train_features) > 0:
            self.model_params.train_feature = model_setting.train_features.split(
                ',')
        self.model_params.random_state = model_setting.seed
        self.sync_file_list = {}
        if self.algorithm_type == AlgorithmType.Train.name:
            self.set_sync_file()

    def set_model_params(self, model_setting: ModelSetting):
        """设置lr参数"""
        self.model_params.set_model_setting(model_setting)

    def get_model_params(self):
        """获取lr参数"""
        return self.model_params

    def set_sync_file(self):
        self.sync_file_list['summary_evaluation'] = [self.summary_evaluation_file, self.remote_summary_evaluation_file]
        self.sync_file_list['train_ks_table'] = [self.train_metric_ks_table, self.remote_train_metric_ks_table]
        self.sync_file_list['train_metric_roc'] = [self.train_metric_roc_file, self.remote_train_metric_roc_file]
        self.sync_file_list['train_metric_ks'] = [self.train_metric_ks_file, self.remote_train_metric_ks_file]
        self.sync_file_list['train_metric_pr'] = [self.train_metric_pr_file, self.remote_train_metric_pr_file]
        self.sync_file_list['train_metric_acc'] = [self.train_metric_acc_file, self.remote_train_metric_acc_file]
        self.sync_file_list['test_ks_table'] = [self.test_metric_ks_table, self.remote_test_metric_ks_table]
        self.sync_file_list['test_metric_roc'] = [self.test_metric_roc_file, self.remote_test_metric_roc_file]
        self.sync_file_list['test_metric_ks'] = [self.test_metric_ks_file, self.remote_test_metric_ks_file]
        self.sync_file_list['test_metric_pr'] = [self.test_metric_pr_file, self.remote_test_metric_pr_file]
        self.sync_file_list['test_metric_acc'] = [self.test_metric_acc_file, self.remote_test_metric_acc_file]


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
