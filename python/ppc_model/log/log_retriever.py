# -*- coding: utf-8 -*-
import subprocess
import logging
from ppc_model.common.protocol import LOG_START_FLAG_FORMATTER, LOG_END_FLAG_FORMATTER
from ppc_common.deps_services.storage_api import StorageApi
from ppc_model.common.base_context import BaseContext
import os


class LogRetriever:
    # max retrieve 500k
    max_retrieved_log_size = 500 * 1024

    def __init__(self, logger, job_cache_dir, storage_client: StorageApi, log_path):
        self.logger = logger
        self.job_cache_dir = job_cache_dir
        self.storage_client = storage_client
        self.relative_log_path = log_path

    def retrieve_log(self, job_id, user):
        try:
            ctx = BaseContext(
                job_id=job_id, job_temp_dir=self.job_cache_dir, user=user)
            # get the log size
            log_size_byte = self.storage_client.get_data(
                self._get_remote_log_size_path(ctx))
            log_size = int.from_bytes(log_size_byte, 'big')
            remote_log_path = os.path.join(
                self.storage_client.get_home_path(), self._get_remote_log_path(ctx))
            if log_size > LogRetriever.max_retrieved_log_size:
                return (log_size, remote_log_path, None)
            # get the log data
            log_data = self.storage_client.get_data(
                self._get_remote_log_path(ctx)).decode(encoding='utf-8')
            return (log_size, remote_log_path, log_data)
        except:
            return (None, None, None)

    def _get_remote_log_path(self, ctx: BaseContext):
        return ctx.remote_log_path

    def _get_remote_log_size_path(self, ctx: BaseContext):
        return ctx.remote_log_size_path

    def _get_local_job_log_path(self, job_id):
        return os.path.join(self.job_cache_dir, job_id, f'{job_id}.log')

    def upload_log(self, job_id, user):
        ret = self._record_model_job_log(job_id)
        if ret is False:
            return
        ctx = BaseContext(
            job_id=job_id, job_temp_dir=self.job_cache_dir, user=user)
        # the log size
        log_size = os.path.getsize(self._get_local_job_log_path(job_id))
        # store the log size
        self.storage_client.save_data(log_size.to_bytes((log_size.bit_length() + 7) // 8, 'big'),
                                      self._get_remote_log_size_path(ctx))
        # store the log path
        self.storage_client.upload_file(self._get_local_job_log_path(
            job_id), self._get_remote_log_path(ctx), user)

    def _make_local_log_dir(self, job_id):
        local_log_path = self._get_local_job_log_path(job_id)
        if os.path.exists(local_log_path):
            return
        parent_dir = os.path.dirname(local_log_path)
        if os.path.exists(parent_dir):
            return
        os.makedirs(parent_dir)

    def _record_model_job_log(self, job_id):
        log_file = self._get_log_file_path()
        if log_file is None or log_file == "":
            current_working_dir = os.getcwd()
            log_file = os.path.join(
                current_working_dir, self.relative_log_path)
        start_keyword = LOG_START_FLAG_FORMATTER.format(job_id=job_id)
        end_keyword = LOG_END_FLAG_FORMATTER.format(job_id=job_id)
        self._make_local_log_dir(job_id)

        command = f"grep -i {job_id} {log_file} > {self._get_local_job_log_path(job_id)}"
        result = subprocess.run(
            command, shell=True, text=True, capture_output=True)
        if result.stderr:
            self.logger.warn(
                f"record_model_job_log for job {job_id} failed, error: {result.stderr}")
            return False
        return True

    def _get_log_file_path(self):
        log_file_path = None
        for handler in self.logger.handlers:
            if isinstance(handler, logging.FileHandler):
                log_file_path = handler.baseFilename
                break
        return log_file_path
