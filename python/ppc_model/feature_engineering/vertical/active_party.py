import multiprocessing
import os
import time

import pandas as pd

from ppc_common.ppc_protos.generated.ppc_model_pb2 import EncAggrLabelsList
from ppc_common.ppc_utils import utils
from ppc_model.common.protocol import PheMessage
from ppc_model.feature_engineering.feature_engineering_context import FeatureEngineeringContext, FeMessage
from ppc_model.feature_engineering.vertical.utils import is_continuous_feature, calculate_woe_iv, \
    calculate_woe_iv_with_pos_event
from ppc_model.interface.model_base import VerticalModel


class VerticalFeatureEngineeringActiveParty(VerticalModel):

    def __init__(self, ctx: FeatureEngineeringContext):
        super().__init__(ctx)
        # 记录woe和iv详情
        self.woe_iv_df = pd.DataFrame(columns=['feature', 'bins', 'count', 'pos_event', 'pos_event_rate',
                                               'neg_event', 'neg_event_rate', 'woe', 'iv', 'iv_total'])
        # 记录特征筛选情况
        self.iv_selected_df = pd.DataFrame(columns=['feature', 'iv_selected'])

    def fit(self, *args, **kwargs) -> None:
        log = self.ctx.components.logger()
        task_id = self.ctx.task_id
        start_time = time.time()
        if self.ctx.use_iv:
            log.info(f"Start feature engineering, task_id: {task_id}, shape: {self.ctx.feature.shape}, "
                     f"feature_name_list: {self.ctx.feature_name_list}")

            if len(self.ctx.feature_name_list) != 0:
                # 明文计算己方特征的woe/iv
                self._compute_self_woe_iv()

            # 密态交互计算合作方特征的woe/iv
            log.info(f"Start enc labels, task_id: {task_id}")
            enc_start_time = time.time()
            enc_labels = self.ctx.phe.encrypt_batch_parallel(self.ctx.label)
            log.info(
                f"Enc labels finished, task_id: {task_id}, count: {len(enc_labels)}, "
                f"time_costs: {time.time() - enc_start_time}s")

            for i in range(1, len(self.ctx.participant_id_list)):
                self._compute_partner_woe_iv(enc_labels, i)

            # 处理计算结果
            self._save_and_sync_fe_results()

            log.info(
                f"Feature engineering finished, task_id: {task_id}, time_costs: {time.time() - start_time}s, "
                f"iv_selected: {self.iv_selected_df}")

    def _compute_self_woe_iv(self):
        log = self.ctx.components.logger()
        start_time = time.time()
        for i in range(self.ctx.feature.shape[1]):
            field = self.ctx.feature_name_list[i]
            is_continuous = is_continuous_feature(self.ctx.categorical, field)
            grouped, iv_total = calculate_woe_iv(self.ctx.feature[:, i],
                                                 self.ctx.label,
                                                 self.ctx.group_num,
                                                 is_continuous)
            for index, row in grouped.iterrows():
                self.woe_iv_df = pd.concat([self.woe_iv_df, pd.DataFrame({
                    'feature': field,
                    'bins': index,
                    'count': [row['count']],
                    'pos_event': [row['pos_event']],
                    'pos_event_rate': [row['pos_event_rate']],
                    'neg_event': [row['neg_event']],
                    'neg_event_rate': [row['neg_event_rate']],
                    'woe': [row['woe']],
                    'iv': [row['iv']],
                    'iv_total': [row['iv_total']]
                })], ignore_index=True)

            self.iv_selected_df.loc[len(self.iv_selected_df)] = {'feature': field,
                                                                 'iv_selected': int(iv_total >= self.ctx.iv_thresh)}
        log.info(
            f"Computing self woe/iv finished, task_id: {self.ctx.task_id}, time_costs: {time.time() - start_time}s")

    def _compute_partner_woe_iv(self, enc_labels, partner_index):
        log = self.ctx.components.logger()
        start_time = time.time()

        partner_id = self.ctx.participant_id_list[partner_index]
        self._send_enc_labels(enc_labels, partner_id)
        enc_aggr_labels = self._get_all_enc_aggr_labels(partner_id)

        # results = []
        # with ProcessPoolExecutor(max_workers=max(1, os.cpu_count() - 1)) as executor:
        #     futures = [executor.submit(
        #         self._process_one_feature,
        #         self.ctx.phe,
        #         field,
        #         count_list,
        #         enc_aggr_labels) for field, count_list, enc_aggr_labels in enc_aggr_labels]
        #     for future in as_completed(futures):
        #         results.append(future.result())

        pool = multiprocessing.Pool()
        tasks = [(self.ctx.phe, field, count_list, enc_aggr_labels) for field, count_list, enc_aggr_labels in
                 enc_aggr_labels]
        results = pool.starmap(self._process_one_feature, tasks)
        pool.close()
        pool.join()

        for field, field_woe_iv_df, iv_total in results:
            # 记录新字段的woe和iv
            self.woe_iv_df = pd.concat(
                [self.woe_iv_df, field_woe_iv_df], ignore_index=True)
            self.iv_selected_df.loc[len(self.iv_selected_df)] = {'feature': field,
                                                                 'iv_selected': int(iv_total >= self.ctx.iv_thresh)}
        log.info(
            f"Computing {partner_id}'s woe/iv finished, task_id: {self.ctx.task_id}, "
            f"time_costs: {time.time() - start_time}s")

    @staticmethod
    def _process_one_feature(phe, field, count_list, enc_aggr_labels):
        pos_event = phe.decrypt_batch(enc_aggr_labels)
        field_woe_iv_df = pd.DataFrame({'bins': range(len(count_list)), 'count': count_list,
                                        'pos_event': pos_event, 'feature': field})
        field_woe_iv_df, iv_total = calculate_woe_iv_with_pos_event(
            field_woe_iv_df)
        return field, field_woe_iv_df, iv_total

    def _get_all_enc_aggr_labels(self, partner_id):
        log = self.ctx.components.logger()
        start_time = time.time()
        data = self.ctx.model_router.pop(task_id=self.ctx.task_id,
                                         task_type=FeMessage.AGGR_LABELS.value, from_inst=partner_id)
        enc_aggr_labels_list_pb = EncAggrLabelsList()
        utils.bytes_to_pb(enc_aggr_labels_list_pb, data)
        public_key = self.ctx.codec.decode_enc_key(
            enc_aggr_labels_list_pb.public_key)

        res = []
        for enc_aggr_labels_pb in enc_aggr_labels_list_pb.enc_aggr_labels_list:
            enc_aggr_labels = [self.ctx.codec.decode_cipher(public_key,
                                                            cipher.ciphertext,
                                                            cipher.exponent
                                                            ) for cipher in enc_aggr_labels_pb.cipher_list]
            field = enc_aggr_labels_pb.field
            res.append(
                (field, list(enc_aggr_labels_pb.count_list), enc_aggr_labels))

        log.info(
            f"All enc aggr labels received, task_id: {self.ctx.task_id}, feature_num: {len(res)}, "
            f"size: {len(data) / 1024}KB, time_costs: {time.time() - start_time}s")
        return res

    def _send_enc_labels(self, enc_labels, receiver):
        log = self.ctx.components.logger()
        start_time = time.time()

        data = PheMessage.packing_data(
            self.ctx.codec, self.ctx.phe.public_key, enc_labels)
        self.ctx.model_router.push(task_id=self.ctx.task_id,
                                   task_type=FeMessage.ENC_LABELS.value,
                                   dst_agency=receiver,
                                   payload=data)
        log.info(
            f"Sending enc labels to {receiver} finished, task_id: {self.ctx.task_id}, label_num: {len(enc_labels)}, "
            f"size: {len(data) / 1024}KB, time_costs: {time.time() - start_time}s")

    def _save_and_sync_fe_results(self):
        log = self.ctx.components.logger()
        task_id = self.ctx.task_id
        self.woe_iv_df.to_csv(self.ctx.woe_iv_file,
                              sep=',', header=True, index=None)
        self.iv_selected_df.to_csv(
            self.ctx.iv_selected_file, sep=',', header=True, index=None)
        self.ctx.components.storage_client.upload_file(self.ctx.woe_iv_file,
                                                       self.ctx.job_id + os.sep + self.ctx.WOE_IV_FILE)
        log.info(f"Saving fe results finished, task_id: {task_id}")
        with open(self.ctx.woe_iv_file, 'rb') as f:
            woe_iv = f.read()
        with open(self.ctx.iv_selected_file, 'rb') as f:
            iv_selected = f.read()
        for i in range(1, len(self.ctx.participant_id_list)):
            partner_id = self.ctx.participant_id_list[i]
            if partner_id in self.ctx.result_receiver_id_list:
                self.ctx.model_router.push(task_id=self.ctx.task_id,
                                           task_type=FeMessage.WOE_FILE.value,
                                           dst_agency=partner_id,
                                           payload=woe_iv)
            self.ctx.model_router.push(task_id=self.ctx.task_id,
                                       task_type=FeMessage.IV_SELECTED_FILE.value,
                                       dst_agency=partner_id,
                                       payload=iv_selected)
            log.info(f"Sending fe results finished, task_id: {task_id}")
