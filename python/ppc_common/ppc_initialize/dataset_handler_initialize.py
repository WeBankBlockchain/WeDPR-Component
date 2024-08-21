# -*- coding: utf-8 -*-
from ppc_common.ppc_config.sql_storage_config_loader import SQLStorageConfigLoader
from ppc_common.ppc_config.file_chunk_config import FileChunkConfig
from ppc_common.ppc_dataset.dataset_helper_factory import DataSetHelperFactory
from ppc_common.deps_services.mysql_storage import MySQLStorage
from ppc_common.deps_services import storage_loader


class DataSetHandlerInitialize:
    def __init__(self, config, logger):
        self._config = config
        self._logger = logger
        self._init_sql_storage()
        self._init_remote_storage()
        self._init_dataset_factory()

    def _init_sql_storage(self):
        self.sql_storage = MySQLStorage(
            SQLStorageConfigLoader.load(self._config))

    def _init_remote_storage(self):
        self.storage_client = storage_loader.load(self._config, self._logger)

    def _init_dataset_factory(self):
        self.file_chunk_config = FileChunkConfig(self._config)
        self.dataset_helper_factory = DataSetHelperFactory(
            sql_storage=self.sql_storage,
            remote_storage=self.storage_client,
            chunk_config=self.file_chunk_config,
            logger=self._logger)
