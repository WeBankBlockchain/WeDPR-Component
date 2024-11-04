import os
import time
from abc import ABC

import pandas as pd

from ppc_common.ppc_utils import utils
from ppc_model.preprocessing.local_processing.preprocessing import process_dataframe
from ppc_model.preprocessing.processing_context import ProcessingContext
from ppc_model.common.base_context import BaseContext


class LocalProcessingParty(ABC):

    def __init__(self, ctx: ProcessingContext):
        self.ctx = ctx

    def processing(self):
        log = self.ctx.components.logger()
        start = time.time()
        need_psi = self.ctx.need_run_psi
        job_id = self.ctx.job_id
        log.info(
            f"run data preprocessing job, job: {job_id}, need_psi: {need_psi}")
        dataset_path = self.ctx.dataset_path
        dataset_file_path = self.ctx.dataset_file_path
        storage_client = self.ctx.components.storage_client
        job_algorithm_type = self.ctx.job_algorithm_type
        psi_result_path = self.ctx.psi_result_path
        model_prepare_file = self.ctx.model_prepare_file
        storage_client.download_file(dataset_path, dataset_file_path)
        if need_psi and (not utils.file_exists(psi_result_path)):
            storage_client.download_file(
                self.ctx.remote_psi_result_path, psi_result_path)
            self.handle_local_psi_result(
                self.ctx.remote_psi_result_path, psi_result_path)
            log.info(
                f"prepare_xgb_after_psi, make_dataset_to_xgb_data_plus_psi_data, dataset_file_path={dataset_file_path}, "
                f"psi_result_path={dataset_file_path}, model_prepare_file={model_prepare_file}")
        self.make_dataset_to_xgb_data()
        storage_client.upload_file(
            model_prepare_file, job_id + os.sep + BaseContext.MODEL_PREPARE_FILE)
        log.info(f"upload model_prepare_file to hdfs, job_id={job_id}")
        if job_algorithm_type == utils.AlgorithmType.Train.name:
            log.info(f"upload column_info to hdfs, job_id={job_id}")
            storage_client.upload_file(self.ctx.preprocessing_result_file,
                                       job_id + os.sep + self.ctx.PREPROCESSING_RESULT_FILE)
        log.info(
            f"call prepare_xgb_after_psi success, job_id={job_id}, timecost: {time.time() - start}")

    def handle_local_psi_result(self, remote_psi_result_path, local_psi_result_path):
        try:
            log = self.ctx.components.logger()
            log.info(
                f"handle_local_psi_result: start handle_local_psi_result, psi_result_path={local_psi_result_path}")
            with open(local_psi_result_path, 'r+', encoding='utf-8') as psi_result_file:
                content = psi_result_file.read()
                psi_result_file.seek(0, 0)
                psi_result_file.write('id\n' + content)
            log.info(
                f"handle_local_psi_result: call handle_local_psi_result success, psi_result_path={local_psi_result_path}")
            # upload to remote
            self.ctx.components.storage_client.upload_file(
                local_psi_result_path, remote_psi_result_path)
        except BaseException as e:
            log.exception(
                f"handle_local_psi_result: handle_local_psi_result, psi_result_path={local_psi_result_path}, error:{e}")
            raise e

    def make_dataset_to_xgb_data(self):
        log = self.ctx.components.logger()
        dataset_file_path = self.ctx.dataset_file_path
        psi_result_file_path = self.ctx.psi_result_path
        model_prepare_file = self.ctx.model_prepare_file
        log.info(f"dataset_file_path:{dataset_file_path}")
        log.info(f"model_prepare_file:{model_prepare_file}")
        need_run_psi = self.ctx.need_run_psi
        job_id = self.ctx.job_id
        if not utils.file_exists(dataset_file_path):
            raise FileNotFoundError(
                f"dataset_file_path not found: {dataset_file_path}")
        dataset_df = pd.read_csv(dataset_file_path)
        if need_run_psi:
            log.info(f"psi_result_file_path:{psi_result_file_path}")
            psi_data = pd.read_csv(psi_result_file_path,
                                   delimiter=utils.CSV_SEP)
            dataset_df = pd.merge(dataset_df, psi_data, on=[
                                  'id']).sort_values(by='id', ascending=True)

        ppc_job_type = self.ctx.job_algorithm_type
        column_info = process_dataframe(
            dataset_df, self.ctx.model_setting, model_prepare_file, ppc_job_type, job_id, self.ctx)

        column_info_pd = pd.DataFrame(column_info).transpose()
        # 如果是训练任务先写本地
        log.info(f"jobid {job_id}, job_algorithm_type {ppc_job_type}")
        if ppc_job_type == utils.AlgorithmType.Train.name:
            log.info(
                f"write {column_info} to {self.ctx.preprocessing_result_file}")
            column_info_pd.to_csv(
                self.ctx.preprocessing_result_file, sep=utils.CSV_SEP, header=True)
        log.info("finish make_dataset_to_xgb_data_plus_psi_data")
