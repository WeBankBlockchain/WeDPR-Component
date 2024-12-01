# -*- coding: utf-8 -*-
import os
from krbcontext.context import krbContext
from hdfs.ext.kerberos import KerberosClient
from ppc_common.deps_services.hdfs_storage import HdfsStorage
from ppc_common.deps_services.storage_api import HDFSStorageConfig


class Krb5HdfsStorage(HdfsStorage):
    def __init__(self, hdfs_config: HDFSStorageConfig):
        super().__init__(hdfs_config, False)
        self.hdfs_config = hdfs_config
        self.krb5_ctx = krbContext(
            using_keytab=True,
            principal=self.hdfs_config.hdfs_auth_principal,
            keytab_file=self.hdfs_config.hdfs_auth_secret_file_path)

        self.client = KerberosClient(self.hdfs_config.hdfs_url)
        self.client = KerberosClient(
            krb_principal=self.hdfs_config.hdfs_auth_principal,
            krb_keytab=self.hdfs_config.hdfs_auth_secret_file_path,
            krb_ccache_path="/tmp/hdfs",
            hdfs_namenode_address=self.hdfs_config.hdfs_url,
            timeout=10000)
