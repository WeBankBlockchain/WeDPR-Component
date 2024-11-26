from abc import ABC, abstractmethod
from enum import Enum
from typing import AnyStr


class StorageType(Enum):
    HDFS = 'HDFS'


class StorageApi(ABC):
    @abstractmethod
    def download_file(self, storage_path: str, local_file_path: str, enable_cache=False):
        pass

    @abstractmethod
    def upload_file(self, local_file_path: str, storage_path: str, owner=None, group=None):
        pass

    @abstractmethod
    def make_file_path(self, storage_path: str):
        pass

    @abstractmethod
    def delete_file(self, storage_path: str):
        pass

    @abstractmethod
    def save_data(self, data: AnyStr, storage_path: str):
        pass

    @abstractmethod
    def get_data(self, storage_path: str) -> AnyStr:
        pass

    @abstractmethod
    def mkdir(self, storage_path: str):
        pass

    @abstractmethod
    def file_existed(self, storage_path: str) -> bool:
        pass

    @abstractmethod
    def file_rename(self, old_storage_path: str, storage_path: str):
        pass

    @abstractmethod
    def storage_type(self):
        pass

    @abstractmethod
    def get_home_path(self):
        return ""
