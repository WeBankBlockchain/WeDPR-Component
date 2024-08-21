# -*- coding: utf-8 -*-

from ppc_common.deps_services.sql_storage_api import SQLStorageConfig
from ppc_common.ppc_utils import common_func


class SQLStorageConfigLoader:
    POOL_RECYCLE_KEY = "SQL_POOL_RECYCLE_SECONDS"
    POOL_SIZE_KEY = "SQL_POOL_SIZE"
    MAX_OVERFLOW_KEY = "SQL_MAX_OVERFLOW_SIZE"
    POOL_TIMEOUT_KEY = "SQL_POOL_TIMEOUT_SECONDS"
    DATABASE_URL_KEY = "SQLALCHEMY_DATABASE_URI"
    DATABASE_TYPE_KEY = "DB_TYPE"
    DATABASE_NAME_KEY = "DB_NAME"

    @staticmethod
    def load(config):
        db_url = common_func.get_config_value(
            SQLStorageConfigLoader.DATABASE_URL_KEY, None, config, True)
        db_type = common_func.get_config_value(
            SQLStorageConfigLoader.DATABASE_TYPE_KEY, None, config, True)
        db_name = common_func.get_config_value(
            SQLStorageConfigLoader.DATABASE_NAME_KEY, None, config, False)
        pool_recycle = common_func.get_config_value(
            SQLStorageConfigLoader.POOL_RECYCLE_KEY, None, config, False)
        pool_size = common_func.get_config_value(
            SQLStorageConfigLoader.POOL_SIZE_KEY, None, config, False)
        max_overflow = common_func.get_config_value(
            SQLStorageConfigLoader.MAX_OVERFLOW_KEY, None, config, False)
        pool_timeout = common_func.get_config_value(
            SQLStorageConfigLoader.POOL_TIMEOUT_KEY, None, config, False)
        return SQLStorageConfig(
            pool_recycle=pool_recycle,
            pool_size=pool_size,
            max_overflow=max_overflow,
            pool_timeout=pool_timeout,
            url=db_url, db_type=db_type, db_name=db_name)
