# -*- coding: utf-8 -*-
from enum import Enum
from abc import ABC, abstractmethod


class SplitMode(Enum):
    NONE = 0  # not split
    SIZE = 1  # split by size
    LINES = 2  # split by lines


class FileObject(ABC):
    @abstractmethod
    def split(self, split_mode, granularity):
        """
        split large file into many small files with given granularity
        """
        pass

    @abstractmethod
    def download(self, enforce_flush=False):
        """
        download the files
        """
        pass

    @abstractmethod
    def upload(self, split_mode, granularity):
        """
        upload the files
        """
        pass

    @abstractmethod
    def delete(self):
        """
        delete the files
        """
        pass

    @abstractmethod
    def existed(self) -> bool:
        """
        check the file object exist or not
        """
        pass

    @abstractmethod
    def rename(self, storage_path: str):
        """
        rename the file object
        """
        pass

    @abstractmethod
    def hit_local_cache(self):
        """hit the local cache or not
        """
        pass

    @abstractmethod
    def get_local_path(self):
        """get the local path
        """
        pass

    @abstractmethod
    def get_remote_path(self):
        """get the remote path
        """
        pass

    @abstractmethod
    def get_data(self):
        """get data
        """
        pass

    @abstractmethod
    def save_data(self, data, split_mode, granularity):
        pass

    @abstractmethod
    def update_data(self, updated_data, split_mode, granularity):
        pass
