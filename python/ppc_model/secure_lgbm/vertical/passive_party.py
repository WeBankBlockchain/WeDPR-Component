import multiprocessing
import time
import json
import numpy as np
from pandas import DataFrame

from ppc_common.ppc_utils import utils
from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo, IterationRequest
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext, LGBMMessage
from ppc_model.secure_lgbm.vertical.booster import VerticalBooster


class VerticalLGBMPassiveParty(VerticalBooster):

    def __init__(self, ctx: SecureLGBMContext, dataset: SecureDataset) -> None:
        super().__init__(ctx, dataset)
        self.params = ctx.model_params
        self._all_feature_name = []
        self.log = ctx.components.logger()
        self.log.info(
            f'task {self.ctx.task_id}: print all params: {self.params.get_all_params()}')

    def fit(
            self,
            *args,
            **kwargs,
    ) -> None:
        self.log.info(
            f'task {self.ctx.task_id}: Starting the lgbm on the passive party.')
        self._init_passive_data()
        self._test_X_bin = self._split_test_data(
            self.ctx, self.dataset.test_X, self._X_split)

        for _ in range(self.params.n_estimators):
            self._tree_id += 1
            start_time = time.time()
            self.log.info(
                f'task {self.ctx.task_id}: Starting n_estimators-{self._tree_id} in passive party.')

            # 初始化
            instance, used_ghlist, public_key = self._receive_gh_instance_list()
            self.ctx.phe.public_key = public_key
            self.log.info(
                f'task {self.ctx.task_id}: Sampling number: {len(instance)}.')

            # 构建
            tree = self._build_tree(instance, used_ghlist)
            self._trees.append(tree)

            # 预测
            self._predict_tree(tree, self._X_bin,
                               LGBMMessage.PREDICT_LEAF_MASK.value)
            self.log.info(f'task {self.ctx.task_id}: Ending n_estimators-{self._tree_id}, '
                          f'time_costs: {time.time() - start_time}s.')

            # 预测验证集
            self._predict_tree(tree, self._test_X_bin,
                               LGBMMessage.TEST_LEAF_MASK.value)
            if self._iteration_early_stop():
                self.log.info(
                    f"task {self.ctx.task_id}: lgbm early stop after {self._tree_id} iterations.")
                break

        self._end_passive_data()

    def transform(self, transform_data: DataFrame) -> DataFrame:
        ...

    def predict(self, dataset: SecureDataset = None) -> np.ndarray:
        start_time = time.time()
        if dataset is None:
            dataset = self.dataset

        self.params.my_categorical_idx = self._get_categorical_idx(
            dataset.feature_name, self.params.categorical_feature)

        test_X_bin = self._split_test_data(
            self.ctx, dataset.test_X, self._X_split)

        [self._predict_tree(
            tree, test_X_bin, LGBMMessage.VALID_LEAF_MASK.value) for tree in self._trees]
        self.log.info(
            f'task {self.ctx.task_id}: Ending predict, time_costs: {time.time() - start_time}s.')

        self._end_passive_data(is_train=False)

    def _init_passive_data(self):

        # 初始化tree id
        self._tree_id = 0

        # 初始化参与方特征
        self._send_byte_data(self.ctx, LGBMMessage.FEATURE_NAME.value,
                             b''.join(s.encode('utf-8') + b' ' for s in self.dataset.feature_name), 0)
        self.params.my_categorical_idx = self._get_categorical_idx(
            self.dataset.feature_name, self.params.categorical_feature)
        feature_name_bytes = self._receive_byte_data(
            self.ctx, LGBMMessage.FEATURE_NAME.value, 0)
        self._all_feature_name = json.loads(feature_name_bytes.decode('utf-8'))

        # 初始化分桶数据集
        feat_bin = FeatureBinning(self.ctx)
        self._X_bin, self._X_split = feat_bin.data_binning(
            self.dataset.train_X)

    def _receive_gh_instance_list(self):

        self._leaf_id = 0

        instance = np.frombuffer(
            self._receive_byte_data(
                self.ctx, f'{LGBMMessage.INSTANCE.value}_{self._tree_id}', 0), dtype=np.int64)
        public_key, gh = self._receive_enc_data(
            self.ctx, f'{LGBMMessage.ENC_GH_LIST.value}_{self._tree_id}', 0)

        return instance, np.array(gh), public_key

    def _build_tree(self, instance, ghlist, depth=0, weight=0):

        if depth == self.params.max_depth:
            return weight
        if self.params.max_depth < 0 and self._leaf_id >= self.params.num_leaves:
            return weight

        self._leaf_id += 1
        print('tree', self._tree_id, 'leaf', self._leaf_id, 'instance', len(instance),
              'ghlist', len(ghlist))
        best_split_info = self._find_best_split(instance, ghlist)

        if best_split_info.best_gain > 0 and best_split_info.best_gain > self.params.min_split_gain:
            left_mask, right_mask = self._get_leaf_mask(
                best_split_info, instance)

            if (abs(best_split_info.w_left) * sum(left_mask) / self.params.lr) < self.params.min_child_weight or \
                    (abs(best_split_info.w_right) * sum(right_mask) / self.params.lr) < self.params.min_child_weight:
                return weight
            if sum(left_mask) < self.params.min_child_samples or sum(right_mask) < self.params.min_child_samples:
                return weight

            left_tree = self._build_tree(
                instance[left_mask], ghlist[left_mask],
                depth + 1, best_split_info.w_left)
            right_tree = self._build_tree(
                instance[right_mask], ghlist[right_mask],
                depth + 1, best_split_info.w_right)

            return [(best_split_info, left_tree, right_tree)]
        else:
            return weight

    def _predict_tree(self, tree, X_bin, key_type):
        if not isinstance(tree, list):
            return None
        else:
            best_split_info, left_subtree, right_subtree = tree[0]
            if self.ctx.participant_id_list[best_split_info.agency_idx] == \
                    self.ctx.components.config_data['AGENCY_ID']:
                if best_split_info.agency_feature in self.params.my_categorical_idx:
                    left_mask = X_bin[:,
                                      best_split_info.agency_feature] == best_split_info.value
                else:
                    left_mask = X_bin[:,
                                      best_split_info.agency_feature] <= best_split_info.value
                self._send_byte_data(
                    self.ctx,
                    f'{key_type}_{best_split_info.tree_id}_{best_split_info.leaf_id}',
                    left_mask.astype('bool').tobytes(), 0)
            else:
                pass
            left_weight = self._predict_tree(left_subtree, X_bin, key_type)
            right_weight = self._predict_tree(right_subtree, X_bin, key_type)
            return [left_weight, right_weight]

    def _find_best_split(self, instance, ghlist):

        self.log.info(f'task {self.ctx.task_id}: Starting n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} in passive party.')
        if len(instance) > 200000:
            self._get_gh_hist_parallel(instance, ghlist)
        else:
            self._get_gh_hist(instance, ghlist)

        best_split_info_byte = self._receive_byte_data(
            self.ctx, f'{LGBMMessage.SPLIT_INFO.value}_{self._tree_id}_{self._leaf_id}', 0)
        best_split_info = BestSplitInfo()
        utils.bytes_to_pb(best_split_info, best_split_info_byte)

        self.log.info(f'task {self.ctx.task_id}: Ending n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} in passive party.')
        return best_split_info

    def _get_gh_hist_parallel(self, instance, ghlist):

        params = []
        for i in range(len(self.dataset.feature_name)):
            params.append({
                'bins': self._X_bin[:, i],
                'xk_bin': self._X_bin[:, i][instance],
                'enc_gh_list': ghlist,
                'phe': self.ctx.phe
            })

        start_time = time.time()
        self.log.info(f'task {self.ctx.task_id}: Start n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} calculate hist in passive party.')

        # gh_hist = []
        # with ProcessPoolExecutor() as executor:
        #     futures = [executor.submit(self._calculate_hist, context) for context in params]
        # for future in as_completed(futures):
        #     gh_hist.append(future.result())

        pool = multiprocessing.Pool()
        gh_hist = pool.map(self._calculate_hist, params)
        pool.close()
        pool.join()

        self.log.info(f'task {self.ctx.task_id}: End n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} calculate hist time_costs: {time.time() - start_time}s.')
        self._send_enc_data(self.ctx,
                            f'{LGBMMessage.ENC_GH_HIST.value}_{self._tree_id}_{self._leaf_id}',
                            gh_hist, 0, matrix_data=True)

    def _get_gh_hist(self, instance, ghlist):

        gh_hist = []
        start_time = time.time()
        self.log.info(f'task {self.ctx.task_id}: Start n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} calculate hist in passive party.')

        for i in range(len(self.dataset.feature_name)):
            param = {
                'bins': self._X_bin[:, i],
                'xk_bin': self._X_bin[:, i][instance],
                'enc_gh_list': ghlist,
                'phe': self.ctx.phe
            }
            gh_hist.append(self._calculate_hist(param))

        self.log.info(f'task {self.ctx.task_id}: Start n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} calculate hist time_costs: {time.time() - start_time}s.')
        self._send_enc_data(self.ctx,
                            f'{LGBMMessage.ENC_GH_HIST.value}_{self._tree_id}_{self._leaf_id}',
                            gh_hist, 0, matrix_data=True)

    @staticmethod
    def _calculate_hist(param):
        bins = param['bins']
        gh_list = param['enc_gh_list']
        phe = param['phe']
        xk_bin = param['xk_bin']
        ghk_hist = []
        sorted_bins = sorted(set(bins))
        for v in sorted_bins:
            # 处理gk_hist中部分分桶没有样本，直接结算值为明文0的情况
            if len(gh_list[xk_bin == v]) == 0:
                ghk_hist.append(phe.encrypt(0))
            else:
                ghk_hist.append(gh_list[xk_bin == v].sum())
        return ghk_hist

    def _iteration_early_stop(self):

        iteration_request_byte = self._receive_byte_data(
            self.ctx, f'{LGBMMessage.STOP_ITERATION.value}_{self._tree_id}', 0)
        iteration_request = IterationRequest()
        utils.bytes_to_pb(iteration_request, iteration_request_byte)

        return iteration_request.stop

    def _end_passive_data(self, is_train=True):

        if self.ctx.components.config_data['AGENCY_ID'] in self.ctx.result_receiver_id_list:
            if is_train:
                self._train_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_train', 0), dtype=np.float)

                self._test_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_test', 0), dtype=np.float)

            else:
                self._test_praba = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_predict', 0), dtype=np.float)
