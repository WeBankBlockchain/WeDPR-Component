# -*- coding: utf-8 -*-

from ppc_common.ppc_dataset.dataset_helper import DataSetHelper


class DataSetHelperFactory:
    def __init__(self, sql_storage, remote_storage, chunk_config, logger):
        self._sql_storage = sql_storage
        self._remote_storage = remote_storage
        self._chunk_config = chunk_config
        self._logger = logger

    def create(self, dataset_user, dataset_id, dataset_local_path):
        return DataSetHelper(dataset_user=dataset_user, dataset_id=dataset_id,
                             dataset_local_path=dataset_local_path, sql_storage=self._sql_storage,
                             chunk_config=self._chunk_config, logger=self._logger,
                             remote_storage=self._remote_storage)


def upload_dataset(dataset_helper_factory, dataset_user, dataset_id, dataset_local_path, logger, log_keyword):
    dataset_helper = dataset_helper_factory.create(
        dataset_user, dataset_id, dataset_local_path)
    logger.info(
        f"{log_keyword}: Upload dataset: {dataset_helper.get_local_path()} => {dataset_helper.get_remote_path()}")
    dataset_helper.upload_dataset()
    logger.info(
        f"{log_keyword}: Upload dataset success: {dataset_helper.get_local_path()} => {dataset_helper.get_remote_path()}")
    return dataset_helper


def delete_dataset(dataset_helper_factory, dataset_user, dataset_id, logger, log_keyword):
    dataset_helper = dataset_helper_factory.create(
        dataset_user=dataset_user, dataset_id=dataset_id, dataset_local_path=None)
    logger.info(
        f"{log_keyword}: Delete dataset: {dataset_helper.get_remote_path()}")
    dataset_helper.file_object.delete()
    logger.info(
        f"{log_keyword}: Delete dataset success: {dataset_helper.get_remote_path()}")
    return dataset_helper


def download_dataset(dataset_helper_factory, dataset_user, dataset_id, dataset_local_path, logger, log_keyword, enforce_flush=False):
    dataset_helper = dataset_helper_factory.create(
        dataset_user, dataset_id, dataset_local_path)
    logger.info(
        f"{log_keyword}: Download dataset: {dataset_helper.get_remote_path()}=>{dataset_helper.get_local_path()}")
    dataset_helper.download_dataset(enforce_flush)
    logger.info(
        f"{log_keyword} dataset success: {dataset_helper.get_remote_path()}=>{dataset_helper.get_local_path()}")
    return dataset_helper


def get_dataset(dataset_helper_factory, dataset_user, dataset_id, logger, log_keyword):
    dataset_helper = dataset_helper_factory.create(
        dataset_user, dataset_id, None)
    logger.info(
        f"{log_keyword}: Get dataset from: {dataset_helper.get_remote_path()}")
    data = dataset_helper.file_object.get_data()
    logger.info(
        f"{log_keyword}: Get dataset success from: {dataset_helper.get_remote_path()}")
    return data
