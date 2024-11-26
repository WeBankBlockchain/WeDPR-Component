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
from ppc_model.network.wedpr_model_transport import ModelRouter
from ppc_model.common.base_context import BaseContext


class ResultFileHandling:

    def __init__(self, ctx: Context) -> None:
        self.ctx = ctx
        self.log = ctx.components.logger()

        if ctx.algorithm_type == AlgorithmType.Train.name:
            self._process_fe_result()

        # Synchronization result file
        if (len(ctx.result_receiver_id_list) == 1 and ctx.participant_id_list[0] != ctx.result_receiver_id_list[0]) \
                or len(ctx.result_receiver_id_list) > 1:
            self._sync_result_files()

        # remove job workspace
        self._remove_workspace()

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
                              self.ctx.selected_col_file,
                              self.ctx.remote_selected_col_file,
                              self.ctx.user)

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
    def _upload_file(storage_client, local_file,
                     remote_file, owner=None,
                     group=None):
        if storage_client is not None:
            storage_client.upload_file(local_file, remote_file, owner, group)

    @staticmethod
    def _download_file(storage_client, local_file, remote_file):
        if storage_client is not None and not os.path.exists(local_file):
            storage_client.download_file(remote_file, local_file)

    @staticmethod
    def make_graph_data(components, ctx: BaseContext, graph_file_name):
        graph_format = 'svg+xml'
        # download with cache
        remote_file_path = ctx.get_remote_file_path(graph_file_name)

        local_file_path = ctx.get_local_file_path(graph_file_name)
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

    def get_remote_path(components, ctx: BaseContext, csv_file_name):
        remote_file_path = ctx.get_remote_file_path(csv_file_name)
        if components.storage_client.get_home_path() is None:
            return remote_file_path
        return os.path.join(components.storage_client.get_home_path(), remote_file_path)

    @staticmethod
    def make_csv_data(components, ctx: BaseContext, csv_file_name):
        import pandas as pd
        from io import StringIO
        remote_file_path = ctx.get_remote_file_path(csv_file_name)
        local_file_path = ctx.get_local_file_path(csv_file_name)
        components.storage_client.download_file(
            remote_file_path, local_file_path, True)
        file_bytes = None
        with open(local_file_path, 'r') as file:
            file_content = file.read()
            file_bytes = file_content.encode('utf-8')
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
        for key, value in self.ctx.sync_file_list.items():
            self.sync_result_file(
                self.ctx, self.ctx.model_router, value[0], value[1], key, self.ctx.user)

    @staticmethod
    def sync_result_file(ctx, model_router: ModelRouter, local_file, remote_file, key_file, user):
        if ctx.role == TaskRole.ACTIVE_PARTY:
            with open(local_file, 'rb') as f:
                byte_data = f.read()
            for partner_index in range(1, len(ctx.participant_id_list)):
                if ctx.participant_id_list[partner_index] in ctx.result_receiver_id_list:
                    SendMessage._send_byte_data(model_router, ctx, f'{CommonMessage.SYNC_FILE.value}_{key_file}',
                                                byte_data, partner_index)
        else:
            if ctx.components.config_data['AGENCY_ID'] in ctx.result_receiver_id_list:
                byte_data = SendMessage._receive_byte_data(model_router, ctx,
                                                           f'{CommonMessage.SYNC_FILE.value}_{key_file}', 0)
                with open(local_file, 'wb') as f:
                    f.write(byte_data)
                ResultFileHandling._upload_file(
                    ctx.components.storage_client,
                    local_file, remote_file, user)


class CommonMessage(Enum):
    SYNC_FILE = "SYNC_FILE"
    EVAL_SET_FILE = "EVAL_SET_FILE"


class SendMessage:

    @staticmethod
    def _send_byte_data(model_router: ModelRouter, ctx, key_type, byte_data, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]
        model_router.push(task_id=ctx.task_id, task_type=key_type,
                          dst_agency=partner_id, payload=byte_data)
        log.info(
            f"task {ctx.task_id}: Sending {key_type} to {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")

    @staticmethod
    def _receive_byte_data(model_router: ModelRouter, ctx, key_type, partner_index):
        log = ctx.components.logger()
        start_time = time.time()
        partner_id = ctx.participant_id_list[partner_index]
        byte_data = model_router.pop(
            task_id=ctx.task_id, task_type=key_type, from_inst=partner_id)
        log.info(
            f"task {ctx.task_id}: Received {key_type} from {partner_id} finished, "
            f"data_size: {len(byte_data) / 1024}KB, time_costs: {time.time() - start_time}s")
        return byte_data
