# -*- coding: utf-8 -*-

import unittest
from ppc_common.deps_services.mysql_storage import MySQLStorage
from ppc_common.deps_services.sql_storage_api import SQLStorageConfig
from ppc_common.db_models.file_object_meta import FileObjectMeta
import threading


class MySQLStorageWrapper:
    def __init__(self):
        # use the default config
        self.engine_url = "mysql://root:12345678@127.0.0.1:3306/ppc?autocommit=true&charset=utf8mb4"
        self.sql_storage_config = SQLStorageConfig(url=self.engine_url)
        self.sql_storage = MySQLStorage(self.sql_storage_config)

    def single_thread_test(self, thread_name, path, ut_obj):
        print(f"# begin test for thread: {thread_name}, path: {path}")
        record_num = 100
        # insert records
        file_count_set = set()
        for i in range(0, record_num):
            tmp_path = path + "_" + str(i)
            file_object_meta = FileObjectMeta(file_path=tmp_path, file_count=i)
            self.sql_storage.merge(file_object_meta)
            file_count_set.add(i)
        # query records
        result_list = self.sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path.startswith(path))
        ut_obj.assertEqual(result_list.count(), record_num)
        # check the file_count
        for item in result_list:
            ut_obj.assertTrue(item.file_count in file_count_set)
            # check the path
            expected_path = path + "_" + str(item.file_count)
            ut_obj.assertEqual(item.file_path, expected_path)
            file_count_set.remove(item.file_count)
        ut_obj.assertEqual(len(file_count_set), 0)
        # update the file_count
        delta = 100
        for i in range(0, record_num):
            tmp_path = path + "_" + str(i)
            file_count = i + delta
            file_object_meta = FileObjectMeta(
                file_path=tmp_path, file_count=file_count)
            self.sql_storage.merge(file_object_meta)
            file_count_set.add(file_count)
        # query and check
        result_list = self.sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path.startswith(path))
        ut_obj.assertEqual(result_list.count(), record_num)
        for item in result_list:
            ut_obj.assertTrue(item.file_count in file_count_set)
            # check the path
            expected_path = path + "_" + str(item.file_count - delta)
            ut_obj.assertEqual(item.file_path, expected_path)
            file_count_set.remove(item.file_count)
        ut_obj.assertEqual(len(file_count_set), 0)
        # delete test
        tmp_path = path + "_0"
        self.sql_storage.delete(
            FileObjectMeta, FileObjectMeta.file_path == tmp_path)
        result_list = self.sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path == tmp_path)
        ut_obj.assertEqual(result_list.count(), 0)
        result_list = self.sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path.startswith(path))
        ut_obj.assertEqual(result_list.count(), record_num - 1)
        # delete all data throw raw sql
        self.sql_storage.execute(
            f"delete from t_file_object where file_path like '{path}%'")
        result_list = self.sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path.startswith(path))
        ut_obj.assertEqual(result_list.count(), 0)
        print(f"# test for thread: {thread_name}, path: {path} success")


class TestMySQLStorage(unittest.TestCase):
    def test_single_thread(self):
        wrapper = MySQLStorageWrapper()
        path = "a/b/c/test_single_thread.csv"
        wrapper.single_thread_test(path, "single_thread", self)

    def test_multi_thread(self):
        loops = 5
        for j in range(loops):
            thread_list = []
            thread_num = 20
            path = "a/b/c/test_multi_thread.csv"
            wrapper = MySQLStorageWrapper()
            for i in range(thread_num):
                thread_name = "job_" + str(i)
                tmp_path = "thread_" + str(i) + "_" + path
                t = threading.Thread(target=wrapper.single_thread_test, name=thread_name, args=(
                    thread_name, tmp_path, self,))
                thread_list.append(t)
            for t in thread_list:
                t.start()
            for t in thread_list:
                t.join()


if __name__ == '__main__':
    unittest.main()
