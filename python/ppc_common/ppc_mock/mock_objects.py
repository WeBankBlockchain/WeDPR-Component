from datetime import datetime
from typing import AnyStr

from ppc_common.deps_services.storage_api import StorageApi


class MockLogger:
    @staticmethod
    def debug(msg):
        current_time = datetime.now()
        print(f"{current_time}, Debug: {msg}")

    @staticmethod
    def info(msg):
        current_time = datetime.now()
        print(f"{current_time}, Info: {msg}")

    @staticmethod
    def warn(msg):
        current_time = datetime.now()
        print(f"{current_time}, Warn: {msg}")

    @staticmethod
    def error(msg):
        current_time = datetime.now()
        print(f"{current_time}, Error: {msg}")


class MockStorageClient(StorageApi):

    def download_file(self, storage_path: str, local_file_path: str, enable_cache=False):
        print(f'download_file: {storage_path} -> {local_file_path}')

    def upload_file(self, local_file_path: str, storage_path: str):
        print(f'upload_file: {storage_path} -> {local_file_path}')

    def make_file_path(self, storage_path: str):
        pass

    def delete_file(self, storage_path: str):
        pass

    def save_data(self, data: AnyStr, storage_path: str):
        pass

    def get_data(self, storage_path: str) -> AnyStr:
        pass

    def mkdir(self, storage_path: str):
        pass

    def file_existed(self, storage_path: str) -> bool:
        pass

    def file_rename(self, old_storage_path: str, storage_path: str):
        pass

    def storage_type(self):
        pass
