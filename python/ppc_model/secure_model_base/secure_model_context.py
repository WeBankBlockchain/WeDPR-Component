# -*- coding: utf-8 -*-
from abc import abstractmethod
from typing import Any, Dict
import json
import os
from ppc_model.common.context import Context
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole
from ppc_common.ppc_utils import common_func
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.common.model_setting import ModelSetting
from ppc_model.common.base_context import BaseContext

from sklearn.base import BaseEstimator


class SecureModel(BaseEstimator):

    def __init__(
            self,
            **kwargs):
        self.train_feature = []
        self.categorical_feature = None
        self.random_state = None
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

    def set_model_setting(self, model_setting: ModelSetting):
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

    def set_params(self, **params: Any):
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


class SecureModelContext(Context):
    def __init__(self,
                 task_id,
                 args,
                 components: Initializer):

        if args['is_label_holder']:
            role = TaskRole.ACTIVE_PARTY
        else:
            role = TaskRole.PASSIVE_PARTY

        super().__init__(job_id=args['job_id'],
                         task_id=task_id,
                         components=components,
                         role=role,
                         user=args['user'])
        self.is_label_holder = args['is_label_holder']
        self.result_receiver_id_list = args['result_receiver_id_list']
        self.participant_id_list = args['participant_id_list']

        model_predict_algorithm_str = common_func.get_config_value(
            "model_predict_algorithm", None, args, False)
        self.model_predict_algorithm = None
        if model_predict_algorithm_str is not None:
            self.model_predict_algorithm = json.loads(
                model_predict_algorithm_str)
        self.algorithm_type = args['algorithm_type']
        self.predict = False
        if self.algorithm_type == AlgorithmType.Predict.name:
            self.predict = True
        # check for the predict task
        if self.predict and self.model_predict_algorithm is None:
            raise f"Not set model_predict_algorithm for the  job: {self.task_id}"

        if 'dataset_id' in args and args['dataset_id'] is not None:
            self.dataset_file_path = os.path.join(
                self.workspace, args['dataset_id'])
        else:
            self.dataset_file_path = None
        # the remote dataset_file_path
        if 'dataset_path' in args:
            self.remote_dataset_path = args['dataset_path']
        if self.remote_dataset_path is None:
            raise f"Must define the dataset_path!"
        # the remote psi_path
        if 'psi_result_path' in args:
            self.remote_psi_path = args['psi_result_path']
        if self.remote_psi_path is None:
            raise f"Must define the psi_result_path"
        self.model_params = self.create_model_param()
        self.reset_model_params(ModelSetting(args['model_dict']))
        # prepare the dataset and psi file
        BaseContext.load_file(storage_client=self.components.storage_client,
                              remote_path=self.remote_dataset_path,
                              local_path=self.dataset_file_path,
                              logger=self.components.logger())
        if self.model_params.use_psi:
            BaseContext.load_file(storage_client=self.components.storage_client,
                                  remote_path=self.remote_psi_path,
                                  local_path=self.psi_result_path,
                                  logger=self.components.logger())
        self.sync_file_list = {}
        if self.algorithm_type == AlgorithmType.Train.name:
            self.set_sync_file()

    @abstractmethod
    def set_sync_file(self):
        pass

    @abstractmethod
    def create_model_param(self):
        pass

    def reset_model_params(self, model_setting: ModelSetting):
        """设置lr参数"""
        self.model_params.set_model_setting(model_setting)
        if model_setting.train_features is not None and len(model_setting.train_features) > 0:
            self.model_params.train_feature = model_setting.train_features.split(
                ',')
        if model_setting.categorical is not None and len(model_setting.categorical) > 0:
            self.model_params.categorical_feature = model_setting.categorical.split(
                ',')
        if model_setting.seed is not None:
            self.model_params.random_state = model_setting.seed
