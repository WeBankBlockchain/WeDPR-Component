import itertools
import time
import json

import numpy as np
from pandas import DataFrame

from ppc_common.deps_services.serialize_type import SerializeType
from ppc_common.ppc_ml.feature.feature_importance import FeatureImportanceStore
from ppc_common.ppc_ml.feature.feature_importance import FeatureImportanceType
from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo, IterationRequest
from ppc_common.ppc_utils import utils
from ppc_model.datasets.data_reduction.feature_selection import FeatureSelection
from ppc_model.datasets.data_reduction.sampling import Sampling
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.metrics.evaluation import Evaluation
from ppc_model.metrics.loss import BinaryLoss
from ppc_model.secure_lr.secure_lr_context import SecureLRContext, LRMessage
from ppc_model.secure_lr.vertical.booster import VerticalBooster


class VerticalLRActiveParty(VerticalBooster):

    def __init__(self, ctx: SecureLRContext, dataset: SecureDataset) -> None:
        super().__init__(ctx, dataset)
        self.params = ctx.model_params
        self._loss_func = BinaryLoss()
        self._all_feature_name = [dataset.feature_name]
        self._all_feature_num = len(dataset.feature_name)
        self.log = ctx.components.logger()
        self.storage_client = ctx.components.storage_client
        self.log.info(
            f'task {self.ctx.task_id}: print all params: {self.params.get_all_params()}')

    def fit(
            self,
            *args,
            **kwargs,
    ) -> None:
        self.log.info(
            f'task {self.ctx.task_id}: Starting the lr on the active party.')
        self._init_active_data()
        
        max_iter = self._init_iter(self.dataset.train_X.shape[0], 
                                   self.params.epochs, self.params.batch_size)
        for _ in range(max_iter):
            self._iter_id += 1
            start_time = time.time()
            self.log.info(
                f'task {self.ctx.task_id}: Starting iter-{self._iter_id} in active party.')

            # 初始化
            idx, feature_select = self._init_each_iter()
            self.log.info(
                f'task {self.ctx.task_id}: feature select: {len(feature_select)}, {feature_select}.')

            # 构建
            self._build_iter(feature_select, idx)

        # 预测
        self._train_praba = self._predict_tree(self.dataset.train_X, LRMessage.PREDICT_LEAF_MASK.value)
        # print('train_praba', set(self._train_praba))

        # 评估
        if not self.params.silent and self.dataset.train_y is not None:
            auc = Evaluation.fevaluation(
                self.dataset.train_y, self._train_praba)['auc']
            self.log.info(
                f'task {self.ctx.task_id}: iter-{self._iter_id}, auc: {auc}.')
        self.log.info(f'task {self.ctx.task_id}: Ending iter-{self._iter_id}, '
                        f'time_costs: {time.time() - start_time}s.')

        # 预测验证集
        self._test_praba = self._predict_tree(self.dataset.test_X, LRMessage.TEST_LEAF_MASK.value)
        if not self.params.silent and self.dataset.test_y is not None:
            auc = Evaluation.fevaluation(
                self.dataset.test_y, self._test_praba)['auc']
            self.log.info(
                f'task {self.ctx.task_id}: iter-{self._iter_id}, test auc: {auc}.')

        self._end_active_data()

    def transform(self, transform_data: DataFrame) -> DataFrame:
        ...

    def predict(self, dataset: SecureDataset = None) -> np.ndarray:
        start_time = time.time()
        if dataset is None:
            dataset = self.dataset

        test_praba = self._predict_tree(dataset.test_X, LRMessage.VALID_LEAF_MASK.value)
        self._test_praba = test_praba

        if dataset.test_y is not None:
            auc = Evaluation.fevaluation(dataset.test_y, test_praba)['auc']
            self.log.info(f'task {self.ctx.task_id}: predict test auc: {auc}.')
        self.log.info(
            f'task {self.ctx.task_id}: Ending predict, time_costs: {time.time() - start_time}s.')

        self._end_active_data(is_train=False)

    def _init_active_data(self):

        # 初始化预测值和权重
        self._train_praba = self._init_praba(self.dataset.train_X.shape[0])
        self._train_weights = self._init_weight(self.dataset.train_X.shape[1])
        self._test_weights = self._init_weight(self.dataset.test_X.shape[1])
        self._iter_id = 0

        # 初始化所有参与方的特征
        for i in range(1, len(self.ctx.participant_id_list)):
            feature_name_bytes = self._receive_byte_data(
                self.ctx, LRMessage.FEATURE_NAME.value, i)
            self._all_feature_name.append(
                [s.decode('utf-8') for s in feature_name_bytes.split(b' ') if s])
            self._all_feature_num += len([s.decode('utf-8')
                                         for s in feature_name_bytes.split(b' ') if s])
        for i in range(1, len(self.ctx.participant_id_list)):
            self._send_byte_data(self.ctx, LRMessage.FEATURE_NAME.value,
                                 json.dumps(self._all_feature_name).encode('utf-8'), i)

        self.log.info(f'task {self.ctx.task_id}: total feature number:{self._all_feature_num}, '
                      f'total feature name: {self._all_feature_name}.')
        self.params.categorical_idx = self._get_categorical_idx(
            list(itertools.chain(*self._all_feature_name)), self.params.categorical_feature)
        self.params.my_categorical_idx = self._get_categorical_idx(
            self.dataset.feature_name, self.params.categorical_feature)

    def _build_iter(self, feature_select, idx):

        x_, y_ = self.dataset.train_X[idx], self.dataset.train_y[idx]

        g = self._loss_func.dot_product(x_, self._train_weights)
        h = 0.5 + self._loss_func.inference(g)
        d = h - y_

        self._send_d_instance_list(d)
        public_key_list, d_other_list, partner_index_list = self._receive_d_instance_list()
        deriv = self._calculate_deriv(x_, d, partner_index_list, d_other_list)

        self._train_weights -= self.params.learning_rate * deriv.astype('float')
        self._train_weights[~np.isin(np.arange(len(self._train_weights)), feature_select)] = 0

    def _predict_tree(self, X, key_type):
        train_g = self._loss_func.dot_product(X, self._train_weights)
        for i in range(1, len(self.ctx.participant_id_list)):
            train_g_other = np.frombuffer(
                self._receive_byte_data(self.ctx, key_type, i), dtype='float')
            train_g += train_g_other
        return self._loss_func.sigmoid(train_g)

    def _end_active_data(self, is_train=True):
        if is_train:
            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LRMessage.PREDICT_PRABA.value}_train',
                                         self._train_praba.astype('float').tobytes(), partner_index)

            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LRMessage.PREDICT_PRABA.value}_test',
                                         self._test_praba.astype('float').tobytes(), partner_index)

        else:
            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LRMessage.PREDICT_PRABA.value}_predict',
                                         self._test_praba.astype('float').tobytes(), partner_index)
