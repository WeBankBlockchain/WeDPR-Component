import os
import shutil
import pandas as pd
import time
from enum import Enum
import base64
from ppc_common.ppc_utils import utils
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.common.context import Context
from ppc_model.common.protocol import TaskRole
from ppc_model.network.stub import PushRequest, PullRequest


class ResultFileHandling:

    def __init__(self, ctx: Context) -> None:
        self.ctx = ctx
        self.log = ctx.components.logger()

        if ctx.algorithm_type == AlgorithmType.Train.name:
            self._process_fe_result()

        # remove job workspace
        # self._remove_workspace()

        # Synchronization result file
        if (len(ctx.result_receiver_id_list) == 1 and ctx.participant_id_list[0] != ctx.result_receiver_id_list[0]) \
                or len(ctx.result_receiver_id_list) > 1:
            self._sync_result_files()

    def _process_fe_result(self):
        if os.path.exists(self.ctx.preprocessing_result_file):
            column_info_fm = pd.read_csv(
                self.ctx.preprocessing_result_file, index_col=0)
            if os.path.exists(self.ctx.iv_selected_file):
                column_info_iv_fm = pd.read_csv(
                    self.ctx.iv_selected_file, index_col=0)
                merged_df = self.union_column_info(
                    column_info_fm, column_info_iv_fm)
            else:
                merged_df = column_info_fm

            merged_df.fillna("None", inplace=True)
            merged_df.to_csv(self.ctx.selected_col_file,
                             sep=utils.CSV_SEP, header=True, index_label='id')
            # 存储column_info到hdfs给前端展示
            self._upload_file(self.ctx.components.storage_client,
                              self.ctx.selected_col_file, self.ctx.remote_selected_col_file)

    @staticmethod
    def union_column_info(column_info1: pd.DataFrame, column_info2: pd.DataFrame):
        """
        union the column_info1 with the column_info2.

        Args:
            column_info1 (DataFrame): The column_info1 to be merged.
            column_info2 (DataFrame): The column_info2 to be merged.

        Returns:
            column_info_merge (DataFrame): The union column_info.
        """
        # 将column_info1和column_info2按照left_index=True, right_index=True的方式进行合并 如果列有缺失则赋值为None 行的顺序按照column_info1
        column_info_conbine = column_info1.merge(
            column_info2, how='outer', left_index=True, right_index=True, sort=False)
        col1_index_list = column_info1.index.to_list()
        col2_index_list = column_info2.index.to_list()
        merged_list = col1_index_list + \
            [item for item in col2_index_list if item not in col1_index_list]
        column_info_conbine = column_info_conbine.reindex(merged_list)
        return column_info_conbine

    @staticmethod
    def _upload_file(storage_client, local_file, remote_file):
        if storage_client is not None:
            storage_client.upload_file(local_file, remote_file)

    @staticmethod
    def _download_file(storage_client, local_file, remote_file):
        if storage_client is not None and not os.path.exists(local_file):
            storage_client.download_file(remote_file, local_file)

    @staticmethod
    def make_graph_data(components, job_id, graph_file_name):
        graph_format = 'svg+xml'
        # download with cache
        remote_file_path = os.path.join(job_id, graph_file_name)
        local_file_path = os.path.join(
            components.job_cache_dir, remote_file_path)
        components.storage_client.download_file(
            remote_file_path, local_file_path, True)
        file_bytes = None
        with open(local_file_path, 'r') as file:
            file_content = file.read()
            file_bytes = file_content.encode('utf-8')
        encoded_data = ""
        if file_bytes is not None:
            encoded_data = base64.b64encode(file_bytes).decode('ascii')
        time.sleep(0.1)
        return f"data:image/{graph_format};base64,{encoded_data}"

    def get_remote_path(components, job_id, csv_file_name):
        if components.storage_client.get_home_path() is None:
            return os.path.join(job_id, csv_file_name)
        return os.path.join(components.storage_client.get_home_path(), job_id, csv_file_name)

    @staticmethod
    def make_csv_data(components, job_id, csv_file_name):
        import pandas as pd
        from io import StringIO
        remote_file_path = os.path.join(job_id, csv_file_name)
        local_file_path = os.path.join(
            components.job_cache_dir, remote_file_path)
        components.storage_client.download_file(
            remote_file_path, local_file_path, True)
        file_bytes = None
        with open(local_file_path, 'r') as file:
            file_content = file.read()
            file_bytes = file_content.encode('utf-8')
        # encoded_data = base64.b64encode(data).decode('ascii')
        # csv_data = np.genfromtxt(data.decode(), delimiter=',')
        # csv_data = np.genfromtxt(StringIO(data.decode()), delimiter=',')
        csv_data = ""
        if file_bytes is not None:
            csv_data = pd.read_csv(StringIO(file_bytes.decode())).astype('str')
        return csv_data

    def _remove_workspace(self):
        if os.path.exists(self.ctx.workspace):
            shutil.rmtree(self.ctx.workspace)
            self.log.info(
                f'job {self.ctx.job_id}: {self.ctx.workspace} has been removed.')
        else:
            self.log.info(
                f'job {self.ctx.job_id}: {self.ctx.workspace} does not exist.')

    def _sync_result_files(self):
        if self.ctx.algorithm_type == AlgorithmType.Train.name:
            self.sync_result_file(self.ctx, self.ctx.metrics_iteration_file,
                                  self.ctx.remote_metrics_iteration_file, 'f1')
            self.sync_result_file(self.ctx, self.ctx.feature_importance_file,
                                  self.ctx.remote_feature_importance_file, 'f2')
            self.sync_result_file(self.ctx, self.ctx.summary_evaluation_file,
                                  self.ctx.remote_summary_evaluation_file, 'f3')
            self.sync_result_file(self.ctx, self.ctx.train_metric_ks_table,
                                  self.ctx.remote_train_metric_ks_table, 'f4')
            self.sync_result_file(self.ctx, self.ctx.train_metric_roc_file,
                                  self.ctx.remote_train_metric_roc_file, 'f5')
            self.sync_result_file(self.ctx, self.ctx.train_metric_ks_file,
                                  self.ctx.remote_train_metric_ks_file, 'f6')
            self.sync_result_file(self.ctx, self.ctx.train_metric_pr_file,
                                  self.ctx.remote_train_metric_pr_file, 'f7')
            self.sync_result_file(self.ctx, self.ctx.train_metric_acc_file,
                                  self.ctx.remote_train_metric_acc_file, 'f8')
            self.sync_result_file(self.ctx, self.ctx.test_metric_ks_table,
                                  self.ctx.remote_test_metric_ks_table, 'f9')
            self.sync_result_file(self.ctx, self.ctx.test_metric_roc_file,
                                  self.ctx.remote_test_metric_roc_file, 'f10')
            self.sync_result_file(self.ctx, self.ctx.test_metric_ks_file,
                                  self.ctx.remote_test_metric_ks_file, 'f11')
            self.sync_result_file(self.ctx, self.ctx.test_metric_pr_file,
                                  self.ctx.remote_test_metric_pr_file, 'f12')
            self.sync_result_file(self.ctx, self.ctx.test_metric_acc_file,
                                  self.ctx.remote_test_metric_acc_file, 'f13')

    @staticmethod
    def sync_result_file(ctx, local_file, remote_file, key_file):
        if ctx.role == TaskRole.ACTIVE_PARTY:
            with open(local_file, 'rb') as f:
                byte_data = f.read()
            for partner_index in range(1, len(ctx.participant_id_list)):
                if ctx.participant_id_list[partner_index] in ctx.result_receiver_id_list:
                    SendMessage._send_byte_data(ctx.components.stub, ctx, f'{CommonMessage.SYNC_FILE.value}_{key_file}',
                                                byte_data, partner_index)
        else:
            if ctx.components.config_data['AGENCY_ID'] in ctx.result_receiver_id_list:
                byte_data = SendMessage._receive_byte_data(ctx.components.stub, ctx,
                                                           f'{CommonMessage.SYNC_FILE.value}_{key_file}', 0)
                with open(local_file, 'wb') as f:
                    f.write(byte_data)
                ResultFileHandling._upload_file(
                    ctx.components.storage_client, local_file, remote_file)


class CommonMessage(Enum):
    SYNC_FILE = "SYNC_FILE"
    EVAL_SET_FILE = "EVAL_SET_FILE"


class SendMessage:

    @staticmethod
    def _send_byte_data(stub, ctx, key_type, byte_data, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        stub.push(PushRequest(
            receiver=partner_id,
            task_id=ctx.task_id,
            key=key_type,
            data=byte_data
        ))

        log.info(
            f"task {ctx.task_id}: Sending {key_type} to {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")

    @staticmethod
    def _receive_byte_data(stub, ctx, key_type, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]

        byte_data = stub.pull(PullRequest(
            sender=partner_id,
            task_id=ctx.task_id,
            key=key_type
        ))

        log.info(
            f"task {ctx.task_id}: Received {key_type} from {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")
        return byte_data
