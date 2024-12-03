# -*- coding: utf-8 -*-
import unittest
from ppc_common.deps_services.storage_api import HDFSStorageConfig
from ppc_common.deps_services.storage_loader import HDFSStorageLoader
import logging


class HDFSStorageWrapper:
    def __init__(self):
        self.logger = logging.getLogger("HDFSStorageWrapper")
        # use the default config
        hdfs_url = "http://127.0.0.1:50070"
        hdfs_user = "root"
        hdfs_home = "/user/ppc"
        enable_krb5_auth = True
        hdfs_auth_principal = "root@NODE.DC1.CONSUL"
        hdfs_auth_password = "root"
        hdfs_hostname_override = "wedpr-0001"
        self.hdfs_config = HDFSStorageConfig(
            hdfs_url=hdfs_url, hdfs_user=hdfs_user,
            hdfs_home=hdfs_home, enable_krb5_auth=enable_krb5_auth,
            hdfs_auth_principal=hdfs_auth_principal,
            hdfs_auth_password=hdfs_auth_password,
            hdfs_hostname_override=hdfs_hostname_override)
        self.hdfs_storage = HDFSStorageLoader.load(
            self.hdfs_config, self.logger)

    def test_file_op(self, file_path):
        hdfs_file_path = f"test/{file_path}"
        print(f"*** upload file test ***")
        self.hdfs_storage.upload_file(
            local_file_path=file_path, hdfs_path=hdfs_file_path)
        print(f"*** upload file test success***")
        print(f"*** download file test ***")
        local_file_path = f"{file_path}.download"
        self.hdfs_storage.download_file(
            hdfs_path=hdfs_file_path, local_file_path=local_file_path)
        print(f"*** download file test success ***")


class TestHDFSStorage(unittest.TestCase):
    def test_file_op(self):
        file_path = "test.csv"
        wrapper = HDFSStorageWrapper()
        wrapper.test_file_op(file_path)


if __name__ == '__main__':
    unittest.main()
