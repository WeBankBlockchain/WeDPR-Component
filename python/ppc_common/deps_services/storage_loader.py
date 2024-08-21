from ppc_common.deps_services.storage_api import StorageType
from ppc_common.deps_services.hdfs_storage import HdfsStorage
from ppc_common.ppc_utils import common_func


def load(config: dict, logger):
    if config['STORAGE_TYPE'] == StorageType.HDFS.value:
        hdfs_user = common_func.get_config_value(
            'HDFS_USER', None, config, False)
        hdfs_home = common_func.get_config_value(
            "HDFS_HOME", None, config, False)
        return HdfsStorage(config['HDFS_ENDPOINT'],  hdfs_user, hdfs_home)
    else:
        raise Exception('unsupported storage type')
