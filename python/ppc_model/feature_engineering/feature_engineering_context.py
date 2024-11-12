from enum import Enum

import numpy as np

from ppc_common.ppc_crypto.phe_factory import PheCipherFactory
from ppc_model.common.context import Context
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole
from ppc_model.common.model_setting import ModelSetting


class FeMessage(Enum):
    ENC_LABELS = "ENC_LABELS"
    AGGR_LABELS = "AGGR_LABELS"
    WOE_FILE = "WOE_FILE"
    IV_SELECTED_FILE = "IV_SELECTED_FILE"


class FeatureEngineeringContext(Context):

    def __init__(self,
                 task_id,
                 args,
                 components: Initializer,
                 role: TaskRole,
                 feature: np.ndarray,
                 feature_name_list: list,
                 label: np.ndarray = None):
        super().__init__(args['job_id'],
                         task_id,
                         components,
                         role)
        self.feature_name_list = feature_name_list
        self.participant_id_list = args['participant_id_list']
        self.result_receiver_id_list = args['result_receiver_id_list']
        self.model_dict = args['model_dict']
        self.feature = feature
        self.label = label
        self.phe = PheCipherFactory.build_phe(
            components.homo_algorithm, components.public_key_length)
        self.codec = PheCipherFactory.build_codec(components.homo_algorithm)
        self.model_setting = ModelSetting(self.model_dict)
        self._parse_model_dict()

    def _parse_model_dict(self):
        self.use_iv = self.model_setting.use_iv
        self.iv_thresh = self.model_setting.iv_thresh
        self.categorical = self.model_setting.categorical
        self.group_num = self.model_setting.group_num
