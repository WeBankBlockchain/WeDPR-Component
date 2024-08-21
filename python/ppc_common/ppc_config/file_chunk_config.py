# -*- coding: utf-8 -*-
from ppc_common.ppc_utils import common_func


class FileChunkConfig:
    # default read chunk size is 512M
    DEFAULT_READ_CHUNK_SIZE = 512
    # default file object chunk size is 2G
    DEFAULT_FILE_OBJECT_CHUNK_SIZE = 2048

    READ_CHUNK_SIZE_MB_KEY = "read_chunk_size_mb"
    FILE_CHUNK_SIZE_MB_KEY = "file_chunk_size_mb"
    ENABLE_ALL_CHUNCK_FILE_MGR_KEY = "enable_file_chunk_on_all_mode"

    def __init__(self, config):
        read_chunk_size = common_func.get_config_value(
            FileChunkConfig.READ_CHUNK_SIZE_MB_KEY,
            FileChunkConfig.DEFAULT_READ_CHUNK_SIZE,
            config, False)
        self.read_chunk_size = int(read_chunk_size) * 1024 * 1024
        file_object_chunk_size = common_func.get_config_value(
            FileChunkConfig.FILE_CHUNK_SIZE_MB_KEY,
            FileChunkConfig.DEFAULT_FILE_OBJECT_CHUNK_SIZE, config, False)
        self.file_object_chunk_size = int(file_object_chunk_size) * 1024 * 1024
        self.enable_file_chunk_on_all_mode = common_func.get_config_value(
            FileChunkConfig.ENABLE_ALL_CHUNCK_FILE_MGR_KEY,
            False, config, False)
