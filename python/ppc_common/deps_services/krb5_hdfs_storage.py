# -*- coding: utf-8 -*-
from hdfs.ext.kerberos import KerberosClient
from ppc_common.deps_services.hdfs_storage import HdfsStorage
from ppc_common.deps_services.storage_api import HDFSStorageConfig


class Krb5HdfsStorage(HdfsStorage):
    def __init__(self, hdfs_config: HDFSStorageConfig, logger):
        super().__init__(hdfs_config, False)
        self.hdfs_config = hdfs_config
        self.client = KerberosClient(
            url=self.hdfs_config.hdfs_url,
            principal=self.hdfs_config.hdfs_auth_principal,
            hostname_override=self.hdfs_config.hdfs_hostname_override,
            password=self.hdfs_config.hdfs_auth_password,
            timeout=10000)
