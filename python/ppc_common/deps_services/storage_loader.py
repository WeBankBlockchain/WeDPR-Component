from ppc_common.deps_services.storage_api import StorageType
from ppc_common.deps_services.storage_api import HDFSStorageConfig
from ppc_common.deps_services.hdfs_storage import HdfsStorage
from ppc_common.deps_services.krb5_hdfs_storage import Krb5HdfsStorage


class HDFSStorageLoader:
    @staticmethod
    def load(hdfs_config: HDFSStorageConfig, logger):
        if hdfs_config.enable_krb5_auth is False:
            return HdfsStorage(hdfs_config)
        return Krb5HdfsStorage(hdfs_config, logger)


def load(config: dict, logger):
    if config['STORAGE_TYPE'] == StorageType.HDFS.value:
        hdfs_config = HDFSStorageConfig()
        hdfs_config.load_config(config, logger)
        return HDFSStorageLoader.load(hdfs_config, logger)
    else:
        raise Exception('unsupported storage type')
