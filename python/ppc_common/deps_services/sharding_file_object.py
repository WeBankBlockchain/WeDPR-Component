# -*- coding: utf-8 -*-
from ppc_common.deps_services.file_object import FileObject, SplitMode
from ppc_common.db_models.file_object_meta import FileObjectMeta
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
import shutil
# from memory_profiler import profile
import os
import time
import uuid
import copy


class FileMeta:
    SUB_FILE_OBJECT_DIR = "object"

    def __init__(self, local_path, remote_path, file_count, logger):
        self.logger = logger
        self._local_path = local_path
        self._remote_path = remote_path
        if self._local_path is None:
            self._local_path = os.path.basename(
                self.remote_path) + "." + str(uuid.uuid4())
            logger.info(f"generate random local path: {self._local_path}")
        self._local_file_dir = os.path.dirname(self._local_path)
        self._local_file_name = os.path.basename(self._local_path)
        if self._remote_path is None:
            self._remote_path = local_path
        if file_count is not None:
            self._file_count = file_count

    @property
    def local_path(self):
        return self._local_path

    @local_path.setter
    def local_path(self, local_path):
        self._local_path = local_path

    @property
    def remote_path(self):
        return self._remote_path

    @remote_path.setter
    def remote_path(self, remote_path):
        self._remote_path = remote_path

    @property
    def file_count(self):
        return self._file_count

    @file_count.setter
    def file_count(self, file_count):
        self._file_count = file_count

    def get_local_sub_file_name(self, file_index):
        return os.path.join(self.get_local_sub_files_dir(), str(file_index))

    def get_local_sub_files_dir(self):
        return os.path.join(
            self._local_file_dir, FileMeta.SUB_FILE_OBJECT_DIR, self._local_file_name)

    def mk_local_sub_files_dir(self):
        local_sub_files_dir = self.get_local_sub_files_dir()
        if os.path.exists(local_sub_files_dir):
            return
        os.makedirs(local_sub_files_dir)

    def remove_local_cache(self):
        local_sub_files_dir = self.get_local_sub_files_dir()
        if os.path.exists(local_sub_files_dir) is False:
            return
        shutil.rmtree(local_sub_files_dir)

    def get_remote_sub_file_name(self, file_index):
        return os.path.join(self._remote_path, str(file_index))


class ShardingFileObject(FileObject):
    def __init__(self, local_path, remote_path, remote_storage_client, sql_storage, logger):
        self.logger = logger
        self._remote_storage_client = remote_storage_client
        self._sql_storage = sql_storage
        self._file_meta = FileMeta(local_path, remote_path, None, logger)
        self.logger.info(
            f"create ShardingFileObject, storage type: {self._remote_storage_client.storage_type()},local path: {self._file_meta.local_path}, remote path: {self._file_meta.remote_path}")

    @property
    def file_meta(self):
        return self._file_meta

    def split(self, split_mode, granularity):
        """
        split large file many small files
        """
        if split_mode is SplitMode.NONE or granularity is None:
            return None
        local_file_size = os.stat(self._file_meta.local_path).st_size
        if local_file_size < granularity:
            self.logger.info(
                f"upload small files directly without split: {self._file_meta.local_path}")
            return None
        if split_mode is SplitMode.SIZE:
            file_list = self._split_by_size(granularity)
        if split_mode is SplitMode.LINES:
            file_list = self._split_by_lines(granularity)
        self._file_meta.file_count = len(file_list)
        return file_list

    # @profile
    def _split_by_size(self, granularity):
        file_index = 0
        start_t = time.time()
        file_list = []
        with open(self._file_meta.local_path, "rb") as fp:
            while True:
                start = time.time()
                fp.seek(file_index * granularity)
                data = fp.read(granularity)
                if not data:
                    break
                self._file_meta.mk_local_sub_files_dir()
                sub_file_path = self._file_meta.get_local_sub_file_name(
                    file_index)
                with open(sub_file_path, "wb") as wfp:
                    wfp.write(data)
                    file_list.append(sub_file_path)
                self.logger.info(
                    f"split file by size, file: {self._file_meta.local_path}, sub_file: {sub_file_path}, time cost: {time.time() - start} seconds")
                file_index += 1
        self.logger.info(
            f"split file by size, file: {self._file_meta.local_path}, split granularity: {granularity}, sub file count: {file_index}, time cost: {time.time() - start_t} seconds")
        return (file_list)

    # @profile
    def _split_by_lines(self, granularity):
        file_index = 0
        self._file_meta.mk_local_sub_files_dir()
        start = time.time()
        file_list = []
        with open(self._file_meta.local_path, "rb") as fp:
            while True:
                start_t = time.time()
                lines = fp.readlines(granularity)
                if not lines:
                    break
                file_name = self._file_meta.get_local_sub_file_name(file_index)
                with open(file_name, "wb") as wfp:
                    wfp.writelines(lines)
                    file_list.append(file_name)
                self.logger.debug(
                    f"split file by lines, file: {self._file_meta.local_path}, sub file path: {file_name}, timecost: {time.time() - start_t} seconds")
                file_index += 1
        self.logger.info(
            f"split file by lines, file: {self._file_meta.local_path}, split granularity: {granularity}, sub file count: {file_index}, timecost: {time.time() - start} seconds")
        return (file_list)

    def _local_cache_miss(self):
        for file_index in range(0, self._file_meta.file_count):
            sub_file_path = self._file_meta.get_local_sub_file_name(file_index)
            if not os.path.exists(sub_file_path):
                return True
        return False

    def _check_uploaded_files(self):
        if self._local_cache_miss():
            error_msg = f"check upload file failed, {self._file_meta.local_path} => {self._file_meta.remote_path}"
            raise PpcException(
                PpcErrorCode.FILE_OBJECT_UPLOAD_CHECK_FAILED.get_code(), error_msg)

    def upload(self, split_mode, granularity):
        """split and upload the file
        """
        if self.split(split_mode, granularity) is not None:
            self._upload_chunks()
            self.logger.info(
                f"Upload success, remove local file cache: {self._file_meta.get_local_sub_files_dir()}")
            self._file_meta.remove_local_cache()
            return
        # upload directly
        start = time.time()
        self.logger.info(
            f"Upload: {self._file_meta.local_path}=>{self._file_meta.remote_path}")
        self._remote_storage_client.upload_file(
            self._file_meta.local_path, self._file_meta.remote_path)
        self.logger.info(
            f"Upload success: {self._file_meta.local_path}=>{self._file_meta.remote_path}, timecost: {time.time() - start}s")

    def _upload_chunks(self):
        """
        upload the files
        """
        start = time.time()
        self._check_uploaded_files()
        for file_index in range(0, self._file_meta.file_count):
            start_t = time.time()
            local_file_path = self._file_meta.get_local_sub_file_name(
                file_index)
            remote_file_path = self._file_meta.get_remote_sub_file_name(
                file_index)
            self.logger.info(f"upload: {local_file_path}=>{remote_file_path}")
            self._remote_storage_client.upload_file(
                local_file_path, remote_file_path)
            self.logger.info(
                f"upload: {local_file_path}=>{remote_file_path} success, timecost: {time.time() - start_t} seconds")
        self.logger.info(
            f"upload: {self._file_meta.local_path}=>{self._file_meta.remote_path} success, timecost: {time.time() - start} seconds, begin to store the meta information")
        start = time.time()
        record = FileObjectMeta(
            file_path=self._file_meta.remote_path, file_count=self._file_meta.file_count)
        self._sql_storage.merge(record)
        self.logger.info(
            f"store meta for {self._file_meta.remote_path} success, timecost: {time.time() - start} seconds")

    def _fetch_file_meta(self):
        file_meta_info = self._sql_storage.query(
            FileObjectMeta, FileObjectMeta.file_path == self._file_meta.remote_path)
        # the file not exists
        if file_meta_info is None or file_meta_info.count() == 0:
            return False
        self.logger.info(
            f"fetch file meta information: {self._remote_storage_client.storage_type()}:{self._file_meta.remote_path}=>{self._file_meta.local_path}, file count: {file_meta_info.first().file_count}")
        self._file_meta.file_count = file_meta_info.first().file_count
        return True

    def download(self, enforce_flush=False):
        """
        download the files
        """
        if enforce_flush is not True and self.hit_local_cache():
            return
        ret = self._fetch_file_meta()
        start = time.time()
        # the remote file not exists
        if ret is False:
            # no sharding case
            if self._remote_storage_client.file_existed(self._file_meta.remote_path):
                self.logger.info(
                    f"Download file: {self._file_meta.remote_path}=>{self._file_meta.local_path}")
                self._remote_storage_client.download_file(
                    self._file_meta.remote_path, self._file_meta.local_path)
                self.logger.info(
                    f"Download file success: {self._file_meta.remote_path}=>{self._file_meta.local_path}, timecost: {time.time() - start}")
                return
            error_msg = f"Download file from {self._remote_storage_client.storage_type()}: {self._file_meta.remote_path} failed for the file not exists!"
            self.logger.error(error_msg)
            raise PpcException(
                PpcErrorCode.FILE_OBJECT_NOT_EXISTS.get_code(), error_msg)
        # remove the local file
        if os.path.exists(self._file_meta.local_path):
            self.logger.info(
                f"Download: remove the existed local file {self._file_meta.local_path}")
            os.remove(self._file_meta.local_path)
        # download from the remote storage client
        start = time.time()
        # merge the file
        offset = 0
        try:
            with open(self._file_meta.local_path, "wb") as fp:
                for file_index in range(0, self._file_meta.file_count):
                    start_t = time.time()
                    fp.seek(offset)
                    remote_file_path = self._file_meta.get_remote_sub_file_name(
                        file_index)
                    local_file_path = self._file_meta.get_local_sub_file_name(
                        file_index)
                    self._remote_storage_client.download_file(
                        remote_file_path, local_file_path)
                    with open(local_file_path, "rb") as f:
                        fp.write(f.read())
                        offset += os.stat(local_file_path).st_size
                    self.logger.info(
                        f"Download: {self._remote_storage_client.storage_type()}:{remote_file_path}=>{local_file_path}, timecost: {time.time() - start_t} seconds")
            # remove the local cache
            self._file_meta.remove_local_cache()
            self.logger.info(
                f"Download: {self._remote_storage_client.storage_type()}:{self._file_meta.remote_path}=>{self._file_meta.local_path} success, file count: {self._file_meta.file_count}, timecost: {time.time() - start} seconds")
        except Exception as e:
            self.logger.warn(
                f"Download: {self._remote_storage_client.storage_type()}:{self._file_meta.remote_path}=>{self._file_meta.local_path} failed, {e}")
            self._remove_local_file()
            raise e

    def _remove_local_file(self):
        if not os.path.exists(self._file_meta.local_path):
            return
        os.remove(self._file_meta.local_path)

    def delete(self):
        """
        delete the files
        """
        if self._fetch_file_meta() is False:
            if self._remote_storage_client.file_existed(self._file_meta.remote_path):
                self._remote_storage_client.delete_file(
                    self._file_meta.remote_path)
                self.logger.info(
                    f"Delete file {self._file_meta.remote_path} success")
                return
            self.logger.info(
                f"Delete nothing for file {self._file_meta.remote_path} not exists")
            return
        start = time.time()
        for file_index in range(0, self._file_meta.file_count):
            start_t = time.time()
            remote_file_path = self._file_meta.get_remote_sub_file_name(
                file_index)
            self._remote_storage_client.delete_file(remote_file_path)
            self.logger.info(
                f"Delete: {self._remote_storage_client.storage_type()}:{remote_file_path}, timecost: {time.time() - start_t} seconds")
        self._delete_remote_dir()
        # delete the record
        self._sql_storage.delete(
            FileObjectMeta, FileObjectMeta.file_path == self._file_meta.remote_path)
        self.logger.info(
            f"Delete: {self._remote_storage_client.storage_type()}:{self._file_meta.remote_path} success, file count: {self._file_meta.file_count}, timecost: {time.time() - start} seconds")

    def _delete_remote_dir(self):
        if self._remote_storage_client.file_existed(self._file_meta.remote_path):
            self._remote_storage_client.delete_file(
                self._file_meta.remote_path)

    def existed(self) -> bool:
        """
        check the file object exist or not
        """
        if self._fetch_file_meta() is True:
            return True
        return self._remote_storage_client.file_existed(self._file_meta.remote_path)

    def hit_local_cache(self):
        """hit the local cache or not
        """
        if not self.existed():
            error_msg = f"The file object: local:{self._file_meta.local_path}, remote: {self._file_meta.remote_path} not exists!"
            self.logger.info(f"find local cache failed: {error_msg}")
            raise PpcException(
                PpcErrorCode.FILE_OBJECT_NOT_EXISTS.get_code(), error_msg)
        return os.path.exists(self._file_meta.local_path)

    def rename(self, storage_path: str):
        """
        rename the file object
        """
        ret = self._fetch_file_meta()
        start = time.time()
        # the remote file not exists
        if ret is False:
            # no sharding case
            if self._remote_storage_client.file_existed(self._file_meta.remote_path):
                self.logger.info(
                    f"Rename file: {self._file_meta.remote_path}=>{storage_path}")
                self._remote_storage_client.file_rename(
                    self._file_meta.remote_path, storage_path)
                self.logger.info(
                    f"Rename file success: {self._file_meta.remote_path}=>{storage_path}, timecost: {time.time() - start} seconds")
                return
            error_msg = f"Rename file {self._remote_storage_client.storage_type()} => {self._file_meta.remote_path} failed for the file not exists!"
            self.logger.error(error_msg)
            raise PpcException(
                PpcErrorCode.FILE_OBJECT_NOT_EXISTS.get_code(), error_msg)
        # rename from the remote storage client
        start = time.time()
        new_file_meta = copy.deepcopy(self._file_meta)
        new_file_meta.remote_path = storage_path
        for file_index in range(0, self._file_meta.file_count):
            start_t = time.time()
            remote_file_path = self._file_meta.get_remote_sub_file_name(
                file_index)
            new_file_path = new_file_meta.get_remote_sub_file_name(
                file_index)
            self._remote_storage_client.file_rename(
                remote_file_path, new_file_path)
            self.logger.info(
                f"Rename: {self._remote_storage_client.storage_type()}:{remote_file_path}=>{new_file_path}, timecost: {time.time() - start_t} seconds")
        # delete the old record
        self._delete_remote_dir()
        self._sql_storage.delete(
            FileObjectMeta, FileObjectMeta.file_path == self._file_meta.remote_path)
        self._file_meta.remote_path = storage_path
        # update the meta
        self.logger.info(
            f"Rename: {self._remote_storage_client.storage_type()}:{self._file_meta.remote_path}=>{storage_path} success, file count: {self._file_meta.file_count}, timecost: {time.time() - start} seconds")
        start = time.time()
        record = FileObjectMeta(file_path=storage_path,
                                file_count=self._file_meta.file_count)
        self._sql_storage.merge(record)
        self.logger.info(
            f"Rename: store meta for {self._file_meta.remote_path} => {storage_path} success, timecost: {time.time() - start} seconds")

    def get_data(self):
        self.download()
        with open(self._file_meta.local_path, 'r') as file:
            file_content = file.read()
            file_bytes = file_content.encode('utf-8')
        os.remove(self._file_meta.local_path)
        return file_bytes

    def save_data(self, data, split_mode, granularity):
        with open(self._file_meta.local_path, 'wb') as file:
            file.write(data)
        self.upload(split_mode, granularity)
        os.remove(self._file_meta.local_path)
        return

    def update_data(self, updated_data, split_mode, granularity):
        start = time.time()
        # try to remove the remote file
        if self.existed():
            self.logger.info(
                f"UpdateData: remove existed remote file: {self._file_meta.remote_path}")
            self.delete()
        # update the data
        self.logger.info(
            f"UpdateData: update the remote file: {self._file_meta.remote_path}")
        self.save_data(data=updated_data if type(updated_data) is bytes else bytes(updated_data, "utf-8"),
                       split_mode=split_mode, granularity=granularity)
        self.logger.info(
            f"UpdateData success: update the remote file: {self._file_meta.remote_path}, timecost: {time.time() - start}")
        return len(updated_data)

    def get_local_path(self):
        """get the local path
        """
        return self._file_meta.local_path

    def get_remote_path(self):
        """get the remote path
        """
        return self._file_meta.remote_path
