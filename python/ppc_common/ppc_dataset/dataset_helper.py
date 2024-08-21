# -*- coding: utf-8 -*-

from ppc_common.deps_services.sharding_file_object import ShardingFileObject
from ppc_common.deps_services.file_object import SplitMode
from ppc_common.deps_services.storage_api import StorageType
import os


class DataSetHelper:
    # default split granularity is 1G
    def __init__(self, dataset_user, dataset_id, dataset_local_path,
                 sql_storage, remote_storage, chunk_config, logger):
        self._split_mode = SplitMode.NONE
        self._chunk_config = chunk_config
        self._chunk_size = self._chunk_config.file_object_chunk_size
        self._dataset_user = dataset_user
        remote_path = os.path.join(dataset_user, dataset_id)
        self._file_object = ShardingFileObject(
            dataset_local_path, remote_path, remote_storage, sql_storage, logger)

    @property
    def file_object(self):
        return self._file_object

    def download_dataset(self, enforce_flush=False):
        """download the dataset
        """
        self._file_object.download(enforce_flush=enforce_flush)

    def upload_dataset(self):
        """upload the dataset
        """
        self._file_object.upload(self._split_mode, self._chunk_size)

    def save_data(self, data):
        """save the dataset
        """
        self._file_object.save_data(
            data=data, split_mode=self._split_mode, granularity=self._chunk_size)

    def update_data(self, data):
        return self._file_object.update_data(
            updated_data=data, split_mode=self._split_mode, granularity=self._chunk_size)

    def file_rename(self, new_storage_path, with_user_home=False):
        if with_user_home:
            self._file_object.rename(os.path.join(
                self._dataset_user, new_storage_path))
        else:
            self._file_object.rename(new_storage_path)

    def get_local_path(self):
        """get the local path
        """
        return self._file_object.get_local_path()

    def get_remote_path(self):
        """get the remote path
        """
        return self._file_object.get_remote_path()
