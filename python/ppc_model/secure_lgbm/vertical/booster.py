import os
import time
import random
import json
import numpy as np

from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.model_crypto.crypto_aes import encrypt_data, decrypt_data, cipher_to_base64, base64_to_cipher
from ppc_model.interface.model_base import VerticalModel
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.common.protocol import PheMessage
from ppc_model.network.stub import PushRequest, PullRequest
from ppc_model.common.model_result import ResultFileHandling
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext, LGBMMessage


# 抽离sgb的公共部分
class VerticalBooster(VerticalModel):
    def __init__(self, ctx: SecureLGBMContext, dataset: SecureDataset) -> None:
        super().__init__(ctx)
        self.dataset = dataset
        self._stub = ctx.components.stub

        self._tree_id = None
        self._leaf_id = None
        self._X_bin = None
        self._X_split = None
        self._trees = []

        self._train_weights = None
        self._train_praba = None
        self._test_weights = None
        self._test_praba = None
        random.seed(ctx.model_params.random_state)
        np.random.seed(ctx.model_params.random_state)

    def _build_tree(self, *args, **kwargs):

        raise NotImplementedError

    def _predict_tree(self, *args, **kwargs):

        raise NotImplementedError

    def _init_praba(self, n):
        return np.full(n, 0.5)

    def _init_weight(self, n):
        return np.zeros(n, dtype=float)

    @staticmethod
    def _get_categorical_idx(feature_name, categorical_feature=[]):
        categorical_idx = []
        if len(categorical_feature) > 0:
            for i in categorical_feature:
                if i in feature_name:
                    categorical_idx.append(feature_name.index(i))
        return categorical_idx

    @staticmethod
    def _compute_gh_sum(feature, value, categorical_idx, gl, hl, grad_hist, hess_hist):
        if feature in categorical_idx:
            gl = grad_hist[feature][value]
            hl = hess_hist[feature][value]
        else:
            gl = gl + grad_hist[feature][value]
            hl = hl + hess_hist[feature][value]
        return gl, hl

    @staticmethod
    def _compute_gain(g, h, gl, hl, gr, hr, λ):
        if (h + λ) != 0 and (hl + λ) != 0 and (hr + λ) != 0:
            return gl**2 / (hl + λ) + gr**2 / (hr + λ) - g**2 / (h + λ)
        else:
            return 0

    @staticmethod
    def _compute_leaf_weight(lr, λ, gl, hl, gr, hr, reg_alpha):

        weight_l = VerticalBooster._calulate_weight(lr, λ, gl, hl, reg_alpha)
        weight_r = VerticalBooster._calulate_weight(lr, λ, gr, hr, reg_alpha)

        return weight_l, weight_r

    @staticmethod
    def _calulate_weight(lr, λ, g, h, reg_alpha):

        # weight = lr * - g / (h + λ)
        if (h + λ) != 0 and g > reg_alpha:
            weight = lr * - (g - reg_alpha) / (h + λ)
        elif (h + λ) != 0 and g < -reg_alpha:
            weight = lr * - (g + reg_alpha) / (h + λ)
        else:
            weight = 0

        return weight

    @staticmethod
    def _get_leaf_instance(X, instance, feature, value, my_categorical_idx):

        if feature in my_categorical_idx:
            left_mask = X[instance, feature] == value
            right_mask = ~left_mask
        else:
            left_mask = X[instance, feature] <= value
            right_mask = ~left_mask

        return left_mask, right_mask

    def _get_leaf_mask(self, split_info, instance):

        if self.ctx.participant_id_list[split_info.agency_idx] == self.ctx.components.config_data['AGENCY_ID']:
            left_mask, right_mask = self._get_leaf_instance(
                self._X_bin, instance, split_info.agency_feature, split_info.value, self.params.my_categorical_idx)
            for partner_index in range(0, len(self.ctx.participant_id_list)):
                if self.ctx.participant_id_list[partner_index] != self.ctx.components.config_data['AGENCY_ID']:
                    self._send_byte_data(
                        self.ctx, f'{LGBMMessage.INSTANCE_MASK.value}_{self._tree_id}_{self._leaf_id}',
                        left_mask.astype('bool').tobytes(), partner_index)
        else:
            left_mask = np.frombuffer(
                self._receive_byte_data(
                    self.ctx, f'{LGBMMessage.INSTANCE_MASK.value}_{self._tree_id}_{self._leaf_id}',
                    split_info.agency_idx), dtype='bool')
            right_mask = ~left_mask

        return left_mask, right_mask

    def _send_enc_data(self, ctx, key_type, enc_data, partner_index, matrix_data=False):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        if matrix_data:
            self._stub.push(PushRequest(
                receiver=partner_id,
                task_id=ctx.task_id,
                key=key_type,
                data=PheMessage.packing_2dim_data(
                    ctx.codec, ctx.phe.public_key, enc_data)
            ))
        else:
            self._stub.push(PushRequest(
                receiver=partner_id,
                task_id=ctx.task_id,
                key=key_type,
                data=PheMessage.packing_data(
                    ctx.codec, ctx.phe.public_key, enc_data)
            ))

        log.info(
            f"task {ctx.task_id}: Sending {key_type} to {partner_id} finished, "
            f"data_length: {len(enc_data)}, time_costs: {time.time() - start_time}s")

    def _receive_enc_data(self, ctx, key_type, partner_index, matrix_data=False):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        byte_data = self._stub.pull(PullRequest(
            sender=partner_id,
            task_id=ctx.task_id,
            key=key_type
        ))

        if matrix_data:
            public_key, enc_data = PheMessage.unpacking_2dim_data(
                ctx.codec, byte_data)
        else:
            public_key, enc_data = PheMessage.unpacking_data(
                ctx.codec, byte_data)

        log.info(
            f"task {ctx.task_id}: Received {key_type} from {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")
        return public_key, enc_data

    def _send_byte_data(self, ctx, key_type, byte_data, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        self._stub.push(PushRequest(
            receiver=partner_id,
            task_id=ctx.task_id,
            key=key_type,
            data=byte_data
        ))

        log.info(
            f"task {ctx.task_id}: Sending {key_type} to {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")

    def _receive_byte_data(self, ctx, key_type, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        byte_data = self._stub.pull(PullRequest(
            sender=partner_id,
            task_id=ctx.task_id,
            key=key_type
        ))

        log.info(
            f"task {ctx.task_id}: Received {key_type} from {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")
        return byte_data

    @staticmethod
    def _split_test_data(ctx, test_X, X_split):
        feat_bin = FeatureBinning(ctx)
        return feat_bin.data_binning(test_X, X_split)[0]

    def save_model(self, file_path=None):
        log = self.ctx.components.logger()
        if file_path is not None:
            self.ctx.feature_bin_file = os.path.join(
                file_path, self.ctx.FEATURE_BIN_FILE)
            self.ctx.model_data_file = os.path.join(
                file_path, self.ctx.MODEL_DATA_FILE)

        if self._X_split is not None and not os.path.exists(self.ctx.feature_bin_file):
            X_split_dict = {k: v for k, v in zip(
                self.dataset.feature_name, self._X_split)}
            with open(self.ctx.feature_bin_file, 'w') as f:
                json.dump(X_split_dict, f)
            ResultFileHandling._upload_file(self.ctx.components.storage_client,
                                            self.ctx.feature_bin_file, self.ctx.remote_feature_bin_file)
            log.info(
                f"task {self.ctx.task_id}: Saved x_split to {self.ctx.feature_bin_file} finished.")

        if not os.path.exists(self.ctx.model_data_file):
            serial_trees = [self._serial_tree(tree) for tree in self._trees]
            with open(self.ctx.model_data_file, 'w') as f:
                json.dump(serial_trees, f)
            ResultFileHandling._upload_file(self.ctx.components.storage_client,
                                            self.ctx.model_data_file, self.ctx.remote_model_data_file)
            log.info(
                f"task {self.ctx.task_id}: Saved serial_trees to {self.ctx.model_data_file} finished.")

        self.merge_model_file()

    def merge_model_file(self):

        # 加密文件
        lgbm_model = {}
        lgbm_model['model_type'] = 'xgb_model'
        lgbm_model['label_provider'] = self.ctx.participant_id_list[0]
        lgbm_model['label_column'] = 'y'
        lgbm_model['participant_agency_list'] = []
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            agency_info = {
                'agency': self.ctx.participant_id_list[partner_index]}
            agency_info['fields'] = self._all_feature_name[partner_index]
            lgbm_model['participant_agency_list'].append(agency_info)

        lgbm_model['model_dict'] = self.ctx.model_params.get_all_params()
        model_text = {}
        with open(self.ctx.feature_bin_file, 'rb') as f:
            feature_bin_data = f.read()
        with open(self.ctx.model_data_file, 'rb') as f:
            model_data = f.read()
        feature_bin_enc = encrypt_data(self.ctx.key, feature_bin_data)
        model_data_enc = encrypt_data(self.ctx.key, model_data)

        my_agency_id = self.ctx.components.config_data['AGENCY_ID']
        model_text[my_agency_id] = [cipher_to_base64(
            feature_bin_enc), cipher_to_base64(model_data_enc)]

        # 发送&接受文件
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                self._send_byte_data(
                    self.ctx, f'{LGBMMessage.MODEL_DATA.value}_feature_bin',
                    feature_bin_enc, partner_index)
                self._send_byte_data(
                    self.ctx, f'{LGBMMessage.MODEL_DATA.value}_model_data',
                    model_data_enc, partner_index)
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                feature_bin_enc = self._receive_byte_data(
                    self.ctx, f'{LGBMMessage.MODEL_DATA.value}_feature_bin', partner_index)
                model_data_enc = self._receive_byte_data(
                    self.ctx, f'{LGBMMessage.MODEL_DATA.value}_model_data', partner_index)
                model_text[self.ctx.participant_id_list[partner_index]] = \
                    [cipher_to_base64(feature_bin_enc),
                     cipher_to_base64(model_data_enc)]
        lgbm_model['model_text'] = model_text

        # 上传密文模型
        with open(self.ctx.model_enc_file, 'w') as f:
            json.dump(lgbm_model, f)
        ResultFileHandling._upload_file(self.ctx.components.storage_client,
                                        self.ctx.model_enc_file, self.ctx.remote_model_enc_file)
        self.ctx.components.logger().info(
            f"task {self.ctx.task_id}: Saved enc model to {self.ctx.model_enc_file} finished.")

    def split_model_file(self):
        # 传入模型
        my_agency_id = self.ctx.components.config_data['AGENCY_ID']
        model_text = self.ctx.model_predict_algorithm['model_text']
        feature_bin_enc, model_data_enc = [
            base64_to_cipher(i) for i in model_text[my_agency_id]]

        # 解密文件
        feature_bin_data = decrypt_data(self.ctx.key, feature_bin_enc)
        model_data = decrypt_data(self.ctx.key, model_data_enc)
        with open(self.ctx.feature_bin_file, 'wb') as f:
            f.write(feature_bin_data)
        with open(self.ctx.model_data_file, 'wb') as f:
            f.write(model_data)

    def load_model(self, file_path=None):
        log = self.ctx.components.logger()
        if file_path is not None:
            self.ctx.feature_bin_file = os.path.join(
                file_path, self.ctx.FEATURE_BIN_FILE)
            self.ctx.model_data_file = os.path.join(
                file_path, self.ctx.MODEL_DATA_FILE)
        if self.ctx.algorithm_type == AlgorithmType.Predict.name:
            self.ctx.remote_feature_bin_file = os.path.join(
                self.ctx.model_params.training_job_id, self.ctx.FEATURE_BIN_FILE)
            self.ctx.remote_model_data_file = os.path.join(
                self.ctx.model_params.training_job_id, self.ctx.MODEL_DATA_FILE)

        try:
            ResultFileHandling._download_file(self.ctx.components.storage_client,
                                              self.ctx.feature_bin_file, self.ctx.remote_feature_bin_file)
            ResultFileHandling._download_file(self.ctx.components.storage_client,
                                              self.ctx.model_data_file, self.ctx.remote_model_data_file)
        except:
            self.split_model_file()

        with open(self.ctx.feature_bin_file, 'r') as f:
            X_split_dict = json.load(f)
        feature_name = list(X_split_dict.keys())
        x_split = list(X_split_dict.values())
        log.info(
            f"task {self.ctx.task_id}: Load x_split from {self.ctx.feature_bin_file} finished.")
        assert len(feature_name) == len(self.dataset.feature_name)

        with open(self.ctx.model_data_file, 'r') as f:
            serial_trees = json.load(f)
        log.info(
            f"task {self.ctx.task_id}: Load serial_trees from {self.ctx.model_data_file} finished.")

        trees = [self._deserial_tree(tree) for tree in serial_trees]
        self._X_split = x_split
        # self.my_feature_name = feature_name
        self._trees = trees

    @staticmethod
    def _serial_tree(tree):
        if isinstance(tree, list):
            best_split_info, left_tree, right_tree = tree[0]
            best_split_info_list = []
            for field in best_split_info.DESCRIPTOR.fields:
                best_split_info_list.append(
                    getattr(best_split_info, field.name))
            left_tree = VerticalBooster._serial_tree(left_tree)
            right_tree = VerticalBooster._serial_tree(right_tree)
            best_split_info_list.extend([left_tree, right_tree])
            return best_split_info_list
        else:
            return tree

    @staticmethod
    def _deserial_tree(tree_list):
        if isinstance(tree_list, list):
            best_split_info_list = tree_list[:-2]
            left_tree, right_tree = tree_list[-2:]
            best_split_info = BestSplitInfo()
            for i, field in enumerate(best_split_info.DESCRIPTOR.fields):
                setattr(best_split_info, field.name, best_split_info_list[i])
            left_tree = VerticalBooster._deserial_tree(left_tree)
            right_tree = VerticalBooster._deserial_tree(right_tree)
            return [(best_split_info, left_tree, right_tree)]
        else:
            return tree_list

    def get_trees(self):
        return self._trees

    def get_x_split(self):
        return self._X_split

    def get_train_praba(self):
        return self._train_praba

    def get_test_praba(self):
        return self._test_praba

    @staticmethod
    def packing_gh(g_list: np.ndarray, h_list: np.ndarray, expand=1000, mod_length=32, pack_length=20):
        '''
        1. 转正整数
        g和h的梯度值为浮点数, 取值范围: [-1 ~ 1]
        浮点数转整数默乘以 1000(取3位小数)
        按照最高数据量100w样本, g/h求和值上限为 1000 * 10**6 = 10**9
        基于g/h上限, 负数模运算转正数需要加上 2**32 (4.29*10**9)

        2. packing
        g/h负数模运算转为正数后最大值为 2**32-1, 100w样本求和需要预留10**6位
        packing g和h时, 对g乘以10**20, 为h预留总计20位长度。
        '''
        mod_n = 2 ** mod_length
        pos_int_glist = ((g_list * expand).astype('int64') + mod_n) % mod_n
        pos_int_hlist = ((h_list * expand).astype('int64') + mod_n) % mod_n

        gh_list = pos_int_glist.astype(
            'object') * 10**pack_length + pos_int_hlist.astype('object')

        return gh_list

    @staticmethod
    def unpacking_gh(gh_sum_list: np.ndarray, expand=1000, mod_length=32, pack_length=20):
        '''
        1. 拆解g_pos_int_sum和h_pos_int_sum
        2. 还原g_sum和h_sum
        '''

        mod_n = 2 ** mod_length
        g_pos_int_sum = (gh_sum_list // 10**pack_length) % mod_n
        h_pos_int_sum = (gh_sum_list % 10**pack_length) % mod_n

        g_pos_int_sum[g_pos_int_sum > 2**(mod_length-1)] -= mod_n
        h_pos_int_sum[g_pos_int_sum > 2**(mod_length-1)] -= mod_n

        g_hist = (g_pos_int_sum / expand).astype('float')
        h_hist = (h_pos_int_sum / expand).astype('float')

        return g_hist, h_hist
