# -*- coding: utf-8 -*-
from abc import ABC, abstractmethod
from ppc_common.ppc_utils import common_func


class SQLStorageConfig(ABC):
    """
    configuration for sql storage
    """
    DEFAULT_RECYCLE = 3600
    DEFAULT_POOL_SIZE = 16
    DEFAULT_MAX_OVERFLOW = 8
    DEFAULT_POOL_TIMEOUT = 30

    def __init__(self, url, pool_recycle=None, pool_size=None, max_overflow=None, pool_timeout=None, db_type="mysql", db_name=None):
        self.pool_recycle = common_func.get_config_value("pool_recycle",
                                                         SQLStorageConfig.DEFAULT_RECYCLE, pool_recycle, False)
        self.pool_size = common_func.get_config_value("pool_size",
                                                      SQLStorageConfig.DEFAULT_POOL_SIZE, pool_size, False)
        self.max_overflow = common_func.get_config_value("max_overflow",
                                                         SQLStorageConfig.DEFAULT_MAX_OVERFLOW, max_overflow, False)
        self.pool_timeout = common_func.get_config_value("pool_timeout",
                                                         SQLStorageConfig.DEFAULT_POOL_TIMEOUT, pool_timeout, False)
        self.db_type = db_type
        self.engine_url = url
        self.db_name = db_name
        self.pool_pre_ping = True


class SQLStorageAPI:
    @abstractmethod
    def query(self, object, condition):
        """
        query the result
        """
        pass

    @abstractmethod
    def merge(self, record):
        """insert the record into db

        Args:
            record (Any): the record need to be inserted
        """
        pass

    @abstractmethod
    def execute(self, sql: str):
        pass

    @abstractmethod
    def delete(self, object, condition):
        """delete according to condition
        Args:
            object (Any): the object
            condition (Any): the condition
        """
        pass
