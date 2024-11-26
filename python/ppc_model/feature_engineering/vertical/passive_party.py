import multiprocessing
import os
import time

from ppc_common.ppc_protos.generated.ppc_model_pb2 import ModelCipher, EncAggrLabels, EncAggrLabelsList
from ppc_common.ppc_utils import utils
from ppc_model.common.protocol import PheMessage
from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning
from ppc_model.feature_engineering.feature_engineering_context import FeatureEngineeringContext, FeMessage
from ppc_model.feature_engineering.vertical.utils import is_continuous_feature
from ppc_model.interface.model_base import VerticalModel


class VerticalFeatureEngineeringPassiveParty(VerticalModel):

    def __init__(self, ctx: FeatureEngineeringContext):
        super().__init__(ctx)

    def fit(self, *args, **kwargs) -> None:
        log = self.ctx.components.logger()

        task_id = self.ctx.task_id
        start_time = time.time()
        if self.ctx.use_iv:
            log.info(f"Start feature engineering, task_id: {task_id}, shape: {self.ctx.feature.shape}, "
                     f"feature_name_list: {self.ctx.feature_name_list}.")

            # 执行密文交互，计算正样本数量
            public_key, enc_labels = self._get_enc_labels()

            # 根据特征分箱，聚合加密标签
            aggr_labels_bytes_list = self._binning_and_aggregating_all(
                public_key, enc_labels)

            # 发送聚合的密文标签
            self._send_all_enc_aggr_labels(public_key, aggr_labels_bytes_list)

            self._get_and_save_result()
            log.info(
                f"Feature engineering finished, task_id: {task_id}, timecost: {time.time() - start_time}s")

    def _get_enc_labels(self):
        log = self.ctx.components.logger()
        start_time = time.time()
        active_party = self.ctx.participant_id_list[0]
        data = self.ctx.model_router.pop(
            task_id=self.ctx.task_id, task_type=FeMessage.ENC_LABELS.value, from_inst=active_party)
        public_key, enc_labels = PheMessage.unpacking_data(
            self.ctx.codec, data)
        log.info(f"All enc labels received, task_id: {self.ctx.task_id}, label_num: {len(enc_labels)}, "
                 f"size: {len(data) / 1024}KB, timecost: {time.time() - start_time}s")
        return public_key, enc_labels

    def _binning_and_aggregating_all(self, public_key, enc_labels) -> list:
        log = self.ctx.components.logger()
        start_time = time.time()
        params = []
        for i in range(self.ctx.feature.shape[1]):
            is_continuous = is_continuous_feature(
                self.ctx.categorical, self.ctx.feature_name_list[i])
            params.append({
                'is_continuous': is_continuous,
                'feature_index': i,
                'field': self.ctx.feature_name_list[i],
                'feature': self.ctx.feature[:, i],
                'public_key': public_key,
                'enc_labels': enc_labels,
                'group_num': self.ctx.group_num,
                'codec': self.ctx.codec
            })
        # aggr_labels_str = []
        # with ProcessPoolExecutor(max_workers=max(1, os.cpu_count() - 1)) as executor:
        #     futures = [executor.submit(self._binning_and_aggregating_one, param) for param in params]
        #     for future in as_completed(futures):
        #         aggr_labels_str.append(future.result())

        pool = multiprocessing.Pool()
        aggr_labels_str = pool.map(self._binning_and_aggregating_one, params)
        pool.close()
        pool.join()

        log.info(f"Feature binning and aggregating finished, task_id: {self.ctx.task_id}, "
                 f"feature_num: {len(params)}, timecost: {time.time() - start_time}s")
        return aggr_labels_str

    @staticmethod
    def _binning_and_aggregating_one(param):
        feature = param['feature']
        if param['is_continuous']:
            bins = FeatureBinning.binning_continuous_feature(
                feature, param['group_num'])[0]
        else:
            bins = FeatureBinning.binning_categorical_feature(feature)[0]

        enc_labels = param['enc_labels']
        data_dict = {}
        for key, value in zip(bins, enc_labels):
            if key in data_dict:
                data_dict[key]['count'] += 1
                # 执行同态加法
                data_dict[key]['sum'] = data_dict[key]['sum'] + value
            else:
                data_dict[key] = {'count': 1, 'sum': value}

        count_list = [data_dict[key]['count']
                      for key in sorted(data_dict.keys())]
        aggr_enc_labels = [data_dict[key]['sum']
                           for key in sorted(data_dict.keys())]

        return VerticalFeatureEngineeringPassiveParty._encode_enc_aggr_labels(
            param['codec'], param['field'], count_list, aggr_enc_labels)

    @staticmethod
    def _encode_enc_aggr_labels(codec, field, count_list, aggr_enc_labels):
        enc_aggr_labels_pb = EncAggrLabels()
        enc_aggr_labels_pb.field = field
        for count in count_list:
            enc_aggr_labels_pb.count_list.append(count)
        for cipher in aggr_enc_labels:
            model_cipher = ModelCipher()
            model_cipher.ciphertext, model_cipher.exponent = \
                codec.encode_cipher(cipher, be_secure=False)
            enc_aggr_labels_pb.cipher_list.append(model_cipher)
        return utils.pb_to_bytes(enc_aggr_labels_pb)

    def _send_all_enc_aggr_labels(self, public_key, aggr_labels_bytes_list):
        start_time = time.time()
        enc_aggr_labels_list_pb = EncAggrLabelsList()
        enc_aggr_labels_list_pb.public_key = self.ctx.codec.encode_enc_key(
            public_key)

        for aggr_labels_bytes in aggr_labels_bytes_list:
            enc_aggr_labels_pb = EncAggrLabels()
            utils.bytes_to_pb(enc_aggr_labels_pb, aggr_labels_bytes)
            enc_aggr_labels_list_pb.enc_aggr_labels_list.append(
                enc_aggr_labels_pb)

        data = utils.pb_to_bytes(enc_aggr_labels_list_pb)

        self.ctx.components.logger().info(
            f"Encoding all enc aggr labels finished, task_id: {self.ctx.task_id}, "
            f"size: {len(data) / 1024}KB, timecost: {time.time() - start_time}s")
        self.ctx.model_router.push(task_id=self.ctx.task_id,
                                   task_type=FeMessage.AGGR_LABELS.value,
                                   dst_agency=self.ctx.participant_id_list[0],
                                   payload=data)
        self.ctx.components.logger().info(
            f"Sending all enc aggr labels finished, task_id: {self.ctx.task_id}, "
            f"feature_num: {len(aggr_labels_bytes_list)}, "
            f"size: {len(data) / 1024}KB, timecost: {time.time() - start_time}s")

    def _get_and_save_result(self):
        active_party = self.ctx.participant_id_list[0]
        if self.ctx.components.transport.self_agency_id in self.ctx.result_receiver_id_list:
            # 保存来自标签方的woe/iv结果
            data = self.ctx.model_router.pop(
                task_id=self.ctx.task_id, task_type=FeMessage.WOE_FILE.value, from_inst=active_party)
            self.ctx.components.logger().info(
                f"Result of woe/iv received, task_id: {self.ctx.task_id}, size: {len(data) / 1024}KB")
            with open(self.ctx.woe_iv_file, 'wb') as f:
                f.write(data)
            self.ctx.components.storage_client.upload_file(self.ctx.woe_iv_file,
                                                           self.ctx.remote_woe_iv_file,
                                                           self.ctx.user)

        # 保存来自标签方的iv筛选结果
        data = self.ctx.model_router.pop(
            task_id=self.ctx.task_id, task_type=FeMessage.IV_SELECTED_FILE.value, from_inst=active_party)
        self.ctx.components.logger().info(
            f"Result of iv_select received, task_id: {self.ctx.task_id}, size: {len(data) / 1024}KB")
        with open(self.ctx.iv_selected_file, 'wb') as f:
            f.write(data)
