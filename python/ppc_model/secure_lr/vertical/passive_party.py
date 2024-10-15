import itertools
import multiprocessing
import time
import json
import numpy as np
from pandas import DataFrame

from ppc_common.ppc_utils import utils
from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo, IterationRequest
from ppc_model.datasets.data_reduction.feature_selection import FeatureSelection
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.metrics.loss import BinaryLoss
from ppc_model.secure_lr.secure_lr_context import SecureLRContext, LRMessage
from ppc_model.secure_lr.vertical.booster import VerticalBooster


class VerticalLRPassiveParty(VerticalBooster):

    def __init__(self, ctx: SecureLRContext, dataset: SecureDataset) -> None:
        super().__init__(ctx, dataset)
        self.params = ctx.model_params
        self._all_feature_name = []
        self._loss_func = BinaryLoss()
        self.log = ctx.components.logger()
        self.log.info(
            f'task {self.ctx.task_id}: print all params: {self.params.get_all_params()}')

    def fit(
            self,
            *args,
            **kwargs,
    ) -> None:
        self.log.info(
            f'task {self.ctx.task_id}: Starting the lr on the passive party.')
        self._init_passive_data()

        max_iter = self._init_iter(self.dataset.train_X.shape[0], 
                                   self.params.epochs, self.params.batch_size)
        for _ in range(max_iter):
            self._iter_id += 1
            start_time = time.time()
            self.log.info(
                f'task {self.ctx.task_id}: Starting iter-{self._iter_id} in passive party.')

            # 初始化
            idx, feature_select = self._init_each_iter()
            self.log.info(
                f'task {self.ctx.task_id}: feature select: {len(feature_select)}, {feature_select}.')

            # 构建
            self._build_iter(feature_select, idx)

        # 预测
        self._predict_tree(self.dataset.train_X, LRMessage.PREDICT_LEAF_MASK.value)
        self.log.info(f'task {self.ctx.task_id}: Ending iter-{self._iter_id}, '
                        f'time_costs: {time.time() - start_time}s.')

        # 预测验证集
        self._predict_tree(self.dataset.test_X, LRMessage.TEST_LEAF_MASK.value)

        self._end_passive_data()

    def transform(self, transform_data: DataFrame) -> DataFrame:
        ...

    def predict(self, dataset: SecureDataset = None) -> np.ndarray:
        start_time = time.time()
        if dataset is None:
            dataset = self.dataset

        self._predict_tree(dataset.test_X, LRMessage.VALID_LEAF_MASK.value)
        self.log.info(
            f'task {self.ctx.task_id}: Ending predict, time_costs: {time.time() - start_time}s.')

        self._end_passive_data(is_train=False)

    def _init_passive_data(self):

        # 初始化预测值和权重
        self._train_praba = self._init_praba(self.dataset.train_X.shape[0])
        self._train_weights = self._init_weight(self.dataset.train_X.shape[1])
        self._test_weights = self._init_weight(self.dataset.test_X.shape[1])
        self._iter_id = 0

        # 初始化参与方特征
        self._send_byte_data(self.ctx, LRMessage.FEATURE_NAME.value,
                             b''.join(s.encode('utf-8') + b' ' for s in self.dataset.feature_name), 0)
        self.params.my_categorical_idx = self._get_categorical_idx(
            self.dataset.feature_name, self.params.categorical_feature)
        feature_name_bytes = self._receive_byte_data(
            self.ctx, LRMessage.FEATURE_NAME.value, 0)
        self._all_feature_name = json.loads(feature_name_bytes.decode('utf-8'))

    def _build_iter(self, feature_select, idx):

        x_ = self.dataset.train_X[idx]

        g = self._loss_func.dot_product(x_, self._train_weights)
        h = self._loss_func.inference(g)
        d = h

        self._send_d_instance_list(d)
        public_key_list, d_other_list, partner_index_list = self._receive_d_instance_list()
        deriv = self._calculate_deriv(x_, d, partner_index_list, d_other_list)

        self._train_weights -= self.params.learning_rate * deriv.astype('float')
        self._train_weights[~np.isin(np.arange(len(self._train_weights)), feature_select)] = 0

    def _predict_tree(self, X, key_type):
        train_g = self._loss_func.dot_product(X, self._train_weights)
        self._send_byte_data(self.ctx, f'{key_type}', 
                             train_g.astype('float').tobytes(), 0)

    def _end_passive_data(self, is_train=True):

        if self.ctx.components.config_data['AGENCY_ID'] in self.ctx.result_receiver_id_list:
            if is_train:
                self._train_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LRMessage.PREDICT_PRABA.value}_train', 0), dtype=np.float)

                self._test_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LRMessage.PREDICT_PRABA.value}_test', 0), dtype=np.float)

            else:
                self._test_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LRMessage.PREDICT_PRABA.value}_predict', 0), dtype=np.float)
