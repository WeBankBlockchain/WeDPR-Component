# -*- coding: utf-8 -*-
from ppc_common.deps_services.sql_storage_api import SQLStorageAPI
from sqlalchemy.orm import sessionmaker, scoped_session
from sqlalchemy import create_engine
from sqlalchemy import delete
from sqlalchemy import text
from contextlib import contextmanager


class MySQLStorage(SQLStorageAPI):
    def __init__(self, storage_config):
        self._engine_url = storage_config.engine_url
        self._storage_config = storage_config
        connect_args = {}
        if storage_config.db_name is not None:
            connect_args = {'schema': storage_config.db_name}
        self._mysql_engine = create_engine(self._engine_url, pool_recycle=self._storage_config.pool_recycle,
                                           pool_size=self._storage_config.pool_size, max_overflow=self._storage_config.max_overflow,
                                           pool_timeout=self._storage_config.pool_timeout,
                                           pool_pre_ping=self._storage_config.pool_pre_ping,
                                           connect_args=connect_args)
        self._session_factory = sessionmaker(bind=self._mysql_engine)

    @contextmanager
    def _get_session(self):
        session = self._session_factory()
        try:
            yield session
            session.commit()
        except Exception:
            session.rollback()
            raise
        finally:
            session.close()

    def query(self, object, condition):
        """
        query according to the condition
        """
        with self._get_session() as session:
            return session.query(object).filter(condition)

    def merge(self, record):
        """merge the given record to db
        Args:
            record (Any): the record should been inserted
        """
        with self._get_session() as session:
            session.merge(record)

    def execute(self, sql: str):
        text_sql = text(sql)
        with self._get_session() as session:
            session.execute(text_sql)

    def delete(self, object, condition):
        """delete according to condition
        Args:
            object (Any): the object
            condition (Any): the condition
        """
        stmt = delete(object).where(condition)
        with self._get_session() as session:
            session.execute(stmt)
