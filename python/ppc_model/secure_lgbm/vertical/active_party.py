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
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext, LGBMMessage
from ppc_model.secure_lgbm.monitor.callback import CallbackContainer
from ppc_model.secure_lgbm.monitor.core import Booster
from ppc_model.secure_lgbm.monitor.early_stopping import EarlyStopping
from ppc_model.secure_lgbm.monitor.evaluation_monitor import EvaluationMonitor
from ppc_model.secure_lgbm.vertical.booster import VerticalBooster


class VerticalLGBMActiveParty(VerticalBooster):

    def __init__(self, ctx: SecureLGBMContext, dataset: SecureDataset) -> None:
        super().__init__(ctx, dataset)
        self.params = ctx.model_params
        self._loss_func = BinaryLoss(self.params.objective)
        self._all_feature_name = [dataset.feature_name]
        self._all_feature_num = len(dataset.feature_name)
        self.log = ctx.components.logger()
        self.storage_client = ctx.components.storage_client
        self.feature_importance_store = FeatureImportanceStore(
            FeatureImportanceStore.DEFAULT_IMPORTANCE_LIST, None, self.log)
        self.log.info(
            f'task {self.ctx.task_id}: print all params: {self.params.get_all_params()}')

    def fit(
            self,
            *args,
            **kwargs,
    ) -> None:
        self.log.info(
            f'task {self.ctx.task_id}: Starting the lgbm on the active party.')

        self._init_active_data()
        self._init_valid_data()
        self._init_early_stop()

        for _ in range(self.params.n_estimators):
            self._tree_id += 1
            start_time = time.time()
            self.log.info(
                f'task {self.ctx.task_id}: Starting n_estimators-{self._tree_id} in active party.')

            # 初始化
            feature_select, instance, used_glist, used_hlist = self._init_each_tree()
            self.log.info(f'task {self.ctx.task_id}: Sampling number: {len(instance)}, '
                          f'feature select: {len(feature_select)}, {feature_select}.')

            # 构建
            tree = self._build_tree(
                feature_select, instance, used_glist, used_hlist)
            self._trees.append(tree)
            # print('tree', tree)

            # 预测
            self._train_weights += self._predict_tree(
                tree, self._X_bin, np.ones(self._X_bin.shape[0], dtype=bool), LGBMMessage.PREDICT_LEAF_MASK.value)
            self._train_praba = self._loss_func.sigmoid(self._train_weights)
            # print('train_praba', set(self._train_praba))

            # 评估
            if not self.params.silent and self.dataset.train_y is not None:
                auc = Evaluation.fevaluation(
                    self.dataset.train_y, self._train_praba)['auc']
                self.log.info(
                    f'task {self.ctx.task_id}: n_estimators-{self._tree_id}, auc: {auc}.')
            self.log.info(f'task {self.ctx.task_id}: Ending n_estimators-{self._tree_id}, '
                          f'time_costs: {time.time() - start_time}s.')

            # 预测验证集
            self._test_weights += self._predict_tree(
                tree, self._test_X_bin, np.ones(
                    self._test_X_bin.shape[0], dtype=bool),
                LGBMMessage.TEST_LEAF_MASK.value)
            self._test_praba = self._loss_func.sigmoid(self._test_weights)
            if not self.params.silent and self.dataset.test_y is not None:
                auc = Evaluation.fevaluation(
                    self.dataset.test_y, self._test_praba)['auc']
                self.log.info(
                    f'task {self.ctx.task_id}: n_estimators-{self._tree_id}, test auc: {auc}.')
            if self._iteration_early_stop():
                self.log.info(
                    f"task {self.ctx.task_id}: lgbm early stop after {self._tree_id} iterations.")
                break

        self._end_active_data()

    def transform(self, transform_data: DataFrame) -> DataFrame:
        ...

    def predict(self, dataset: SecureDataset = None) -> np.ndarray:
        start_time = time.time()
        if dataset is None:
            dataset = self.dataset

        self.params.my_categorical_idx = self._get_categorical_idx(
            dataset.feature_name, self.params.categorical_feature)

        test_weights = self._init_weight(dataset.test_X.shape[0])
        test_X_bin = self._split_test_data(
            self.ctx, dataset.test_X, self._X_split)

        for tree in self._trees:
            test_weights += self._predict_tree(
                tree, test_X_bin, np.ones(test_X_bin.shape[0], dtype=bool), LGBMMessage.VALID_LEAF_MASK.value)
        test_praba = self._loss_func.sigmoid(test_weights)
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
        self._train_weights = self._init_weight(self.dataset.train_X.shape[0])
        self._tree_id = 0

        # 初始化所有参与方的特征
        for i in range(1, len(self.ctx.participant_id_list)):
            feature_name_bytes = self._receive_byte_data(
                self.ctx, LGBMMessage.FEATURE_NAME.value, i)
            self._all_feature_name.append(
                [s.decode('utf-8') for s in feature_name_bytes.split(b' ') if s])
            self._all_feature_num += len([s.decode('utf-8')
                                         for s in feature_name_bytes.split(b' ') if s])
        for i in range(1, len(self.ctx.participant_id_list)):
            self._send_byte_data(self.ctx, LGBMMessage.FEATURE_NAME.value,
                                 json.dumps(self._all_feature_name).encode('utf-8'), i)

        self.log.info(f'task {self.ctx.task_id}: total feature number:{self._all_feature_num}, '
                      f'total feature name: {self._all_feature_name}.')
        self.params.categorical_idx = self._get_categorical_idx(
            list(itertools.chain(*self._all_feature_name)), self.params.categorical_feature)
        self.params.my_categorical_idx = self._get_categorical_idx(
            self.dataset.feature_name, self.params.categorical_feature)

        # 更新feature_importance中的特征列表
        self.feature_importance_store.set_init(
            list(itertools.chain(*self._all_feature_name)))

        # 初始化分桶数据集
        feat_bin = FeatureBinning(self.ctx)
        self._X_bin, self._X_split = feat_bin.data_binning(
            self.dataset.train_X)

    def _init_each_tree(self):

        if self.callback_container:
            self.callback_container.before_iteration(self.model)

        gradient = self._loss_func.compute_gradient(
            self.dataset.train_y, self._train_praba)
        hessian = self._loss_func.compute_hessian(self._train_praba)

        feature_select = FeatureSelection.feature_selecting(
            list(itertools.chain(*self._all_feature_name)),
            self.params.train_feature, self.params.feature_rate)
        instance, used_glist, used_hlist = Sampling.sample_selecting(
            gradient, hessian, self.params.subsample,
            self.params.use_goss, self.params.top_rate, self.params.other_rate)
        self._send_gh_instance_list(instance, used_glist, used_hlist)

        return feature_select, instance, used_glist, used_hlist

    def _send_gh_instance_list(self, instance, glist, hlist):

        self._leaf_id = 0
        gh_list = self.packing_gh(glist, hlist)

        start_time = time.time()
        self.log.info(f'task {self.ctx.task_id}: Starting n_estimators-{self._tree_id} '
                      f'encrypt g & h in active party.')
        enc_ghlist = self.ctx.phe.encrypt_batch_parallel(
            (gh_list).astype('object'))
        self.log.info(f'task {self.ctx.task_id}: Finished n_estimators-{self._tree_id} '
                      f'encrypt gradient & hessian time_costs: {time.time() - start_time}.')

        for partner_index in range(1, len(self.ctx.participant_id_list)):
            self._send_byte_data(self.ctx, f'{LGBMMessage.INSTANCE.value}_{self._tree_id}',
                                 instance.astype('int64').tobytes(), partner_index)
            self._send_enc_data(self.ctx, f'{LGBMMessage.ENC_GH_LIST.value}_{self._tree_id}',
                                enc_ghlist, partner_index)

    def _build_tree(self, feature_select, instance, glist, hlist, depth=0, weight=0):

        if depth == self.params.max_depth:
            return weight
        if self.params.max_depth < 0 and self._leaf_id >= self.params.num_leaves:
            return weight

        self._leaf_id += 1
        if self.params.colsample_bylevel > 0 and self.params.colsample_bylevel < 1:
            feature_select_level = sorted(np.random.choice(
                feature_select, size=int(len(feature_select) * self.params.colsample_bylevel), replace=False))
            best_split_info = self._find_best_split(
                feature_select_level, instance, glist, hlist)
        else:
            best_split_info = self._find_best_split(
                feature_select, instance, glist, hlist)

        if best_split_info.best_gain > 0 and best_split_info.best_gain > self.params.min_split_gain:
            gain_list = {FeatureImportanceType.GAIN: best_split_info.best_gain,
                         FeatureImportanceType.WEIGHT: 1}
            self.feature_importance_store.update_feature_importance(
                best_split_info.feature, gain_list)
            left_mask, right_mask = self._get_leaf_mask(
                best_split_info, instance)

            if (abs(best_split_info.w_left) * sum(left_mask) / self.params.lr) < self.params.min_child_weight or \
                    (abs(best_split_info.w_right) * sum(right_mask) / self.params.lr) < self.params.min_child_weight:
                return weight
            if sum(left_mask) < self.params.min_child_samples or sum(right_mask) < self.params.min_child_samples:
                return weight

            left_tree = self._build_tree(
                feature_select, instance[left_mask], glist[left_mask],
                hlist[left_mask], depth + 1, best_split_info.w_left)
            right_tree = self._build_tree(
                feature_select, instance[right_mask], glist[right_mask],
                hlist[right_mask], depth + 1, best_split_info.w_right)

            return [(best_split_info, left_tree, right_tree)]
        else:
            return weight

    def _predict_tree(self, tree, X_bin, leaf_mask, key_type):
        if not isinstance(tree, list):
            return tree * leaf_mask
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
            else:
                left_mask = np.frombuffer(
                    self._receive_byte_data(
                        self.ctx,
                        f'{key_type}_{best_split_info.tree_id}_{best_split_info.leaf_id}',
                        best_split_info.agency_idx), dtype='bool')
            right_mask = ~left_mask
            left_weight = self._predict_tree(
                left_subtree, X_bin, leaf_mask * left_mask, key_type)
            right_weight = self._predict_tree(
                right_subtree, X_bin, leaf_mask * right_mask, key_type)
            return left_weight + right_weight

    def _find_best_split(self, feature_select, instance, glist, hlist):

        self.log.info(f'task {self.ctx.task_id}: Starting n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} in active party.')
        grad_hist, hess_hist = self._get_gh_hist(instance, glist, hlist)
        best_split_info = self._get_best_split_point(
            feature_select, glist, hlist, grad_hist, hess_hist)
        # print('grad_hist_sum', [sum(sublist) for sublist in grad_hist])

        best_split_info.tree_id = self._tree_id
        best_split_info.leaf_id = self._leaf_id
        if best_split_info.best_gain > 0:
            agency_idx, agency_feature = self._get_best_split_agency(
                self._all_feature_name, best_split_info.feature)
            best_split_info.agency_idx = agency_idx
            best_split_info.agency_feature = agency_feature

        for partner_index in range(1, len(self.ctx.participant_id_list)):
            self._send_byte_data(
                ctx=self.ctx,
                key_type=f'{LGBMMessage.SPLIT_INFO.value}_{self._tree_id}_{self._leaf_id}',
                byte_data=utils.pb_to_bytes(best_split_info),
                partner_index=partner_index)
        self.log.info(f'task {self.ctx.task_id}: Ending n_estimators-{self._tree_id} '
                      f'leaf-{self._leaf_id} in active party.')
        # print('best_split_info', best_split_info)
        return best_split_info

    def _get_gh_hist(self, instance, glist, hlist):
        ghist, hhist = self._calculate_hist(
            self._X_bin, instance, glist, hlist)

        for partner_index in range(1, len(self.ctx.participant_id_list)):
            partner_feature_name = self._all_feature_name[partner_index]

            partner_ghist = [None] * len(partner_feature_name)
            partner_hhist = [None] * len(partner_feature_name)
            _, gh_hist = self._receive_enc_data(
                self.ctx, f'{LGBMMessage.ENC_GH_HIST.value}_{self._tree_id}_{self._leaf_id}',
                partner_index, matrix_data=True)

            for feature_index in range(len(partner_feature_name)):
                ghk_hist = np.array(self.ctx.phe.decrypt_batch(
                    gh_hist[feature_index]), dtype='object')
                gk_hist, hk_hist = self.unpacking_gh(ghk_hist)
                partner_ghist[feature_index] = gk_hist
                partner_hhist[feature_index] = hk_hist

            ghist.extend(partner_ghist)
            hhist.extend(partner_hhist)

        return ghist, hhist

    @staticmethod
    def _calculate_hist(X_bin, instance, used_glist, used_hlist):

        g_hist = []
        h_hist = []
        for k in range(X_bin.shape[1]):
            Xk_bin = X_bin[instance, k]
            gk_hist = []
            hk_hist = []
            sorted_x = sorted(set(X_bin[:, k]))
            for v in sorted_x:
                gk_hist.append(used_glist[Xk_bin == v].sum())
                hk_hist.append(used_hlist[Xk_bin == v].sum())
            g_hist.append(gk_hist)
            h_hist.append(hk_hist)

        return g_hist, h_hist

    def _get_best_split_point(self, feature_select, glist, hlist, grad_hist, hess_hist):

        beat_feature, best_value, best_gain, best_wl, best_wr = None, None, 0, None, None
        g = np.sum(glist)
        h = np.sum(hlist)

        for feature in feature_select:
            gl = 0
            hl = 0
            for value in range(len(grad_hist[feature])):
                gl, hl = self._compute_gh_sum(
                    feature, value, self.params.categorical_idx, gl, hl, grad_hist, hess_hist)
                gr = g - gl
                hr = h - hl

                gain = self._compute_gain(g, h, gl, hl, gr, hr, self.params.λ)
                wl, wr = self._compute_leaf_weight(
                    self.params.lr, self.params.λ, gl, hl, gr, hr, self.params.reg_alpha)
                compare = bool(gain > best_gain)
                # print('f', feature, 'v', value, 'gl', gl, 'gr', gr, 'hl', hl, 'hr', hr,
                #       'gain', gain, 'wl', wl, 'wr', wr, 'compare', compare)

                if compare:
                    beat_feature = feature
                    best_value = value
                    best_wl = wl
                    best_wr = wr
                    best_gain = gain

        return BestSplitInfo(feature=beat_feature,
                             value=best_value,
                             best_gain=best_gain,
                             w_left=best_wl,
                             w_right=best_wr)

    @staticmethod
    def _get_best_split_agency(all_feature_name, feature):
        """Get the agency index and feature index of the best split point.

        Parameters
        ----------
        all_feature_name : two-dimensional list
            Feature list of all participating agency.
        feature : int
            Best split point global feature index.

        Returns
        -------
        sublist_index : int
            Agency index.
        position_in_sublist : int
            Feature index.
        """
        count = 0
        for sublist_index, sublist in enumerate(all_feature_name):
            if count + len(sublist) > feature:
                position_in_sublist = feature - count
                return sublist_index, position_in_sublist
            count += len(sublist)
        return None

    def _init_valid_data(self):
        self._test_weights = self._init_weight(self.dataset.test_X.shape[0])
        self._test_X_bin = self._split_test_data(
            self.ctx, self.dataset.test_X, self._X_split)

    def _init_early_stop(self):

        callbacks = []
        early_stopping_rounds = self.params.early_stopping_rounds
        if early_stopping_rounds != 0:
            eval_metric = self.params.eval_metric
            early_stopping = EarlyStopping(
                rounds=early_stopping_rounds, metric_name=eval_metric, save_best=True)
            callbacks.append(early_stopping)

        verbose_eval = self.params.verbose_eval
        if verbose_eval != 0:
            evaluation_monitor = EvaluationMonitor(
                logger=self.log, period=verbose_eval)
            callbacks.append(evaluation_monitor)

        callback_container = None
        if len(callbacks) != 0:
            callback_container = CallbackContainer(
                callbacks=callbacks, feval=Evaluation.fevaluation)

        model = Booster(y_true=self.dataset.train_y, test_y_true=self.dataset.test_y,
                        workspace=self.ctx.workspace, job_id=self.ctx.job_id,
                        storage_client=self.storage_client)

        if callback_container:
            callback_container.before_training(model)

        self.model = model
        self.callback_container = callback_container

    def _iteration_early_stop(self):
        # check early stopping
        early_stopping_rounds = self.params.early_stopping_rounds
        if early_stopping_rounds != 0:
            # evaluate the model using test sets
            eval_on_test = True
            pred = self._test_praba
        else:
            eval_on_test = False
            pred = self._train_praba
        stop = False
        if self.callback_container:
            stop = self.callback_container.after_iteration(model=self.model,
                                                           pred=pred,
                                                           eval_on_test=eval_on_test)
            self.log.info(
                f"task {self.ctx.task_id}: after iteration {self._tree_id} iterations, stop: {stop}.")

        iteration_request = IterationRequest()
        iteration_request.epoch = self._tree_id - 1
        iteration_request.stop = stop

        # send stop to passive
        for partner_index in range(1, len(self.ctx.participant_id_list)):
            self._send_byte_data(
                ctx=self.ctx,
                key_type=f'{LGBMMessage.STOP_ITERATION.value}_{self._tree_id}',
                byte_data=utils.pb_to_bytes(iteration_request),
                partner_index=partner_index)

        return stop

    def _end_active_data(self, is_train=True):
        if is_train:
            self.feature_importance_store.store(
                serialize_type=SerializeType.CSV, local_file_path=self.ctx.feature_importance_file,
                remote_file_path=self.ctx.remote_feature_importance_file, storage_client=self.storage_client)

            if self.callback_container:
                with self.ctx.components.plot_lock:
                    self.callback_container.after_training(self.model)

            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_train',
                                         self._train_praba.astype('float').tobytes(), partner_index)

            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_test',
                                         self._test_praba.astype('float').tobytes(), partner_index)

        else:
            for partner_index in range(1, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] in self.ctx.result_receiver_id_list:
                    self._send_byte_data(self.ctx, f'{LGBMMessage.PREDICT_PRABA.value}_predict',
                                         self._test_praba.astype('float').tobytes(), partner_index)
