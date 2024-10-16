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


class LGBMModel(BaseEstimator):

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

    def set_model_setting(self, model_setting: ModelSetting) -> "LGBMModel":
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

    def set_params(self, **params: Any) -> "LGBMModel":
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

    def get_all_params(self):
        """返回SecureLGBMParams所有参数"""
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


class SecureLGBMContext(Context):

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

        self.model_params = SecureLGBMParams()
        model_setting = ModelSetting(args['model_dict'])
        self.set_model_params(model_setting)
        if model_setting.train_features is not None and len(model_setting.train_features) > 0:
            self.model_params.train_feature = model_setting.train_features.split(
                ',')
        if model_setting.categorical is not None and len(model_setting.categorical) > 0:
            self.model_params.categorical_feature = model_setting.categorical.split(
                ',')
        self.model_params.n_estimators = model_setting.num_trees
        self.model_params.feature_rate = model_setting.colsample_bytree
        self.model_params.min_split_gain = model_setting.gamma
        self.model_params.random_state = model_setting.seed

        self.sync_file_list = {}
        if self.algorithm_type == AlgorithmType.Train.name:
            self.set_sync_file()

    def set_model_params(self, model_setting: ModelSetting):
        """设置lgbm参数"""
        self.model_params.set_model_setting(model_setting)

    def get_model_params(self):
        """获取lgbm参数"""
        return self.model_params

    def set_sync_file(self):
        self.sync_file_list['metrics_iteration'] = [self.metrics_iteration_file, self.remote_metrics_iteration_file]
        self.sync_file_list['feature_importance'] = [self.feature_importance_file, self.remote_feature_importance_file]
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
