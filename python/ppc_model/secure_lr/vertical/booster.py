import os
import time
import random
import json
import itertools
import numpy as np

from ppc_common.ppc_protos.generated.ppc_model_pb2 import BestSplitInfo
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.model_crypto.crypto_aes import encrypt_data, decrypt_data, cipher_to_base64, base64_to_cipher
from ppc_model.interface.model_base import VerticalModel
from ppc_model.datasets.data_reduction.feature_selection import FeatureSelection
from ppc_model.datasets.dataset import SecureDataset
from ppc_model.common.protocol import PheMessage
from ppc_model.network.stub import PushRequest, PullRequest
from ppc_model.common.model_result import ResultFileHandling
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.secure_lr.secure_lr_context import SecureLRContext, LRMessage


# 抽离sgb的公共部分
class VerticalBooster(VerticalModel):
    def __init__(self, ctx: SecureLRContext, dataset: SecureDataset) -> None:
        super().__init__(ctx)
        self.dataset = dataset
        self._stub = ctx.components.stub

        self._iter_id = None

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

    @staticmethod
    def _init_praba(n):
        return np.full(n, 0.5)

    @staticmethod
    def _init_weight(n):
        return np.zeros(n, dtype=float)

    @staticmethod
    def _init_iter(n, epochs, batch_size):
        return round(n*epochs/batch_size)

    @staticmethod
    def _get_sample_idx(i, n, size):
        start_idx = (i * size) % n
        end_idx = start_idx + size
        if end_idx <= n:
            idx = list(range(start_idx, end_idx))
        else:
            head_idx = end_idx - n
            idx = list(range(start_idx, n)) + list(range(head_idx))
        return idx

    @staticmethod
    def _get_categorical_idx(feature_name, categorical_feature=[]):
        categorical_idx = []
        if len(categorical_feature) > 0:
            for i in categorical_feature:
                if i in feature_name:
                    categorical_idx.append(feature_name.index(i))
        return categorical_idx

    def _init_each_iter(self):

        idx = self._get_sample_idx(self._iter_id-1, self.dataset.train_X.shape[0], 
                                   size = self.params.batch_size)
        feature_select = FeatureSelection.feature_selecting(
            list(self.dataset.feature_name),
            self.params.train_feature, self.params.feature_rate)

        return idx, feature_select

    def _send_d_instance_list(self, d):

        d_list = self.rounding_d(d)
        my_agency_id = self.ctx.components.config_data['AGENCY_ID']

        start_time = time.time()
        self.log.info(f'task {self.ctx.task_id}: Starting iter-{self._iter_id} '
                      f'encrypt d in {my_agency_id} party.')
        enc_dlist = self.ctx.phe.encrypt_batch_parallel((d_list).astype('object'))
        self.log.info(f'task {self.ctx.task_id}: Finished iter-{self._iter_id} '
                      f'encrypt d time_costs: {time.time() - start_time}.')

        for partner_index in range(len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                self._send_enc_data(self.ctx, f'{LRMessage.ENC_D_LIST.value}_{self._iter_id}',
                                    enc_dlist, partner_index)

    def _receive_d_instance_list(self):

        my_agency_id = self.ctx.components.config_data['AGENCY_ID']
        
        public_key_list = []
        d_other_list = []
        partner_index_list = []
        for partner_index in range(len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                public_key, enc_d = self._receive_enc_data(
                    self.ctx, f'{LRMessage.ENC_D_LIST.value}_{self._iter_id}', partner_index)
                public_key_list.append(public_key)
                d_other_list.append(np.array(enc_d))
                partner_index_list.append(partner_index)

        return public_key_list, d_other_list, partner_index_list

    def _calculate_deriv(self, x_, d, partner_index_list, d_other_list):

        x = self.rounding_d(x_)
        deriv = np.matmul(x_.T, d) / x_.shape[0]
        for i, partner_index in enumerate(partner_index_list):
            # 计算明文*密文 matmul
            # deriv_other_i = np.matmul(x.T, d_other_list[i])
            deriv_other_i = self.enc_matmul(x.T, d_other_list[i])
            
            # 发送密文，接受密文并解密
            self._send_enc_data(self.ctx, f'{LRMessage.ENC_D_HIST.value}_{self._iter_id}',
                                deriv_other_i, partner_index)
            _, enc_deriv_i = self._receive_enc_data(
                self.ctx, f'{LRMessage.ENC_D_HIST.value}_{self._iter_id}', partner_index)
            deriv_i_rec = np.array(self.ctx.phe.decrypt_batch(enc_deriv_i), dtype='object')
            deriv_i = self.recover_d(self.ctx, deriv_i_rec, is_square=True) / x_.shape[0]
            
            # 发送明文，接受明文并计算
            self._send_byte_data(self.ctx, f'{LRMessage.D_MATMUL.value}_{self._iter_id}',
                                 deriv_i.astype('float').tobytes(), partner_index)
            deriv_x_i = np.frombuffer(self._receive_byte_data(
                    self.ctx, f'{LRMessage.D_MATMUL.value}_{self._iter_id}', partner_index), dtype=np.float)
            self.log.info(f'{self.ctx.components.config_data["AGENCY_ID"]}, deriv: {deriv}.')
            self.log.info(f'{self.ctx.components.config_data["AGENCY_ID"]}, deriv_x_i: {deriv_x_i}.')
            deriv += deriv_x_i
        self.log.info(f'{self.ctx.components.config_data["AGENCY_ID"]}, deriv: {deriv}.')
        return deriv

    def _calculate_deriv1(self, x_, d, partner_index_list, d_other_list):

        x = self.rounding_d(x_)
        deriv = np.matmul(x_.T, d) / x_.shape[0]
        for i, partner_index in enumerate(partner_index_list):
            # TODO：重载方法，目前支持np.array(enc_dlist).sum()的方式，不支持明文*密文
            # deriv_other_i = np.matmul(x.T, d_other_list[i])
            deriv_other_i = self.enc_matmul(x.T, d_other_list[i])
            self._send_enc_data(self.ctx, f'{LRMessage.ENC_D_HIST.value}_{self._iter_id}',
                                deriv_other_i, partner_index)
            _, enc_deriv_i = self._receive_enc_data(
                self.ctx, f'{LRMessage.ENC_D_HIST.value}_{self._iter_id}', partner_index)
            deriv_i = np.array(self.ctx.phe.decrypt_batch(enc_deriv_i), dtype='object')
            deriv += (self.recover_d(self.ctx, deriv_i, is_square=True) / x_.shape[0])
        return deriv

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

    def save_model(self, file_path=None):
        log = self.ctx.components.logger()
        if file_path is not None:
            self.ctx.model_data_file = os.path.join(
                file_path, self.ctx.MODEL_DATA_FILE)

        if not os.path.exists(self.ctx.model_data_file):
            serial_weight = list(self._train_weights)
            with open(self.ctx.model_data_file, 'w') as f:
                json.dump(serial_weight, f)
            ResultFileHandling._upload_file(self.ctx.components.storage_client,
                                            self.ctx.model_data_file, self.ctx.remote_model_data_file)
            log.info(
                f"task {self.ctx.task_id}: Saved serial_weight to {self.ctx.model_data_file} finished.")

        self.merge_model_file()

    def merge_model_file(self):

        # 加密文件
        lr_model = {}
        lr_model['model_type'] = 'lr_model'
        lr_model['label_provider'] = self.ctx.participant_id_list[0]
        lr_model['label_column'] = 'y'
        lr_model['participant_agency_list'] = []
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            agency_info = {'agency': self.ctx.participant_id_list[partner_index]}
            agency_info['fields'] = self.ctx._all_feature_name[partner_index]
            lr_model['participant_agency_list'].append(agency_info)
        
        lr_model['model_dict'] = self.ctx.model_params
        model_text = {}
        with open(self.ctx.model_data_file, 'rb') as f:
            model_data = f.read()
        model_data_enc = encrypt_data(self.ctx.key, model_data)
        
        my_agency_id = self.ctx.components.config_data['AGENCY_ID']
        model_text[my_agency_id] = cipher_to_base64(model_data_enc)

        # 发送&接受文件
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                self._send_byte_data(
                    self.ctx, f'{LRMessage.MODEL_DATA.value}_model_data',
                    model_data_enc, partner_index)
        for partner_index in range(0, len(self.ctx.participant_id_list)):
            if self.ctx.participant_id_list[partner_index] != my_agency_id:
                model_data_enc = self._receive_byte_data(
                    self.ctx, f'{LRMessage.MODEL_DATA.value}_model_data', partner_index)
                model_text[self.ctx.participant_id_list[partner_index]] = cipher_to_base64(model_data_enc)
        lr_model['model_text'] = model_text

        # 上传密文模型
        with open(self.ctx.model_enc_file, 'w') as f:
            json.dump(lr_model, f)
        ResultFileHandling._upload_file(self.ctx.components.storage_client,
                                        self.ctx.model_enc_file, self.ctx.remote_model_enc_file)
        self.ctx.components.logger().info(
            f"task {self.ctx.task_id}: Saved enc model to {self.ctx.model_enc_file} finished.")

    def split_model_file(self):
        # 传入模型
        my_agency_id = self.ctx.components.config_data['AGENCY_ID']
        model_text = self.ctx.model_predict_algorithm['model_text']
        model_data_enc = base64_to_cipher(model_text[my_agency_id])

        # 解密文件
        model_data = decrypt_data(self.ctx.key, model_data_enc)
        with open(self.ctx.model_data_file, 'wb') as f:
            f.write(model_data)

    def load_model(self, file_path=None):
        log = self.ctx.components.logger()
        if file_path is not None:
            self.ctx.model_data_file = os.path.join(
                file_path, self.ctx.MODEL_DATA_FILE)
        if self.ctx.algorithm_type == AlgorithmType.Predict.name:
            self.ctx.remote_model_data_file = os.path.join(
                self.ctx.model_params.training_job_id, self.ctx.MODEL_DATA_FILE)

        try:
            ResultFileHandling._download_file(self.ctx.components.storage_client,
                                              self.ctx.model_data_file, self.ctx.remote_model_data_file)
        except:
            self.split_model_file()

        with open(self.ctx.model_data_file, 'r') as f:
            serial_weight = json.load(f)
        self._train_weights = np.array(serial_weight)
        log.info(
            f"task {self.ctx.task_id}: Load serial_weight from {self.ctx.model_data_file} finished.")

    def get_weights(self):
        return self._train_weights

    def get_train_praba(self):
        return self._train_praba

    def get_test_praba(self):
        return self._test_praba

    @staticmethod
    def enc_matmul(arr, enc):
        result = []
        for i in range(len(arr)):
            # arr[i] * enc  # 需要将密文放在前面
            result.append((enc * arr[i]).sum())
        return np.array(result)

    @staticmethod
    def rounding_d(d_list: np.ndarray, expand=1000):
        return (d_list * expand).astype('int')

    @staticmethod
    def recover_d(ctx, d_sum_list: np.ndarray, is_square=False, expand=1000):
        
        d_sum_list[d_sum_list > 2**(ctx.phe.key_length-1)] -= 2**(ctx.phe.key_length)

        if is_square:
            return (d_sum_list / expand / expand).astype('float')
        else:
            return (d_sum_list / expand).astype('float')
