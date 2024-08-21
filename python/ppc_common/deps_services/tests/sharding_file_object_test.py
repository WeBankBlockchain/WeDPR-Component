# -*- coding: utf-8 -*-
import unittest
from ppc_common.deps_services.sharding_file_object import ShardingFileObject
from ppc_common.deps_services.file_object import SplitMode
from ppc_common.deps_services.mysql_storage import MySQLStorage
from ppc_common.deps_services.sql_storage_api import SQLStorageConfig
from ppc_common.deps_services.hdfs_storage import HdfsStorage
import sys
import logging
import pandas as pd


class ShardingFileObjectTestWrapper:
    def __init__(self, ut_obj, local_path, remote_path):
        self.ut_obj = ut_obj
        self.engine_url = "mysql://root:12345678@127.0.0.1:3306/ppc?autocommit=true&charset=utf8mb4"
        self.sql_storage_config = SQLStorageConfig(url=self.engine_url)
        logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
        self.logger = logging.getLogger(__name__)
        self.sql_storage = MySQLStorage(self.sql_storage_config)
        remote_storage_url = "http://127.0.0.1:9870"
        remote_storage_client = HdfsStorage(remote_storage_url, "chenyujie")
        self.sharding_file_object = ShardingFileObject(
            local_path, remote_path, remote_storage_client, self.sql_storage, self.logger)

    def upload(self, split_mode, granularity, expected_lines):
        file_list = self.sharding_file_object.upload(split_mode, granularity)
        if split_mode is SplitMode.LINES:
            self._check_lines(file_list, expected_lines)

    def _check_lines(self, file_list, expected_lines):
        lines = 0
        # check the lines
        i = 0
        columns_info = None
        df = None
        for file in file_list:
            # can be parsed by read_csv
            if i == 0:
                df = pd.read_csv(file, header=0)
                columns_info = df.columns
            else:
                df = pd.read_csv(file, header=None)
                df.columns = columns_info
            lines += len(df)
            i += 1
        self.ut_obj.assertEqual(lines, expected_lines)


class ShardingFileObjectTest(unittest.TestCase):

    def test_split_by_lines(self):
        local_path = "bak/train_test.csv"
        remote_path = "train_test"
        sharding_object_wrapper = ShardingFileObjectTestWrapper(
            self, local_path, remote_path)
        split_mode = SplitMode.LINES
        # 100w
        granularity = 20 * 1024 * 1024
        sharding_object_wrapper.upload(split_mode, granularity, 500000)
        sharding_object_wrapper.sharding_file_object.download()


"""
    def test_split_by_size(self):
        local_path = "bak/train_test.csv"
        remote_path = "train_test"
        sharding_object_wrapper = ShardingFileObjectTestWrapper(
            self, local_path, remote_path)
        split_mode = SplitMode.SIZE
        # 20M
        granularity = 20 * 1024 * 1024
        sharding_object_wrapper.sharding_file_object.split(
            split_mode, granularity)
"""


if __name__ == '__main__':
    unittest.main()
