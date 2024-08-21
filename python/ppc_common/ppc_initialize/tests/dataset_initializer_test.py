# -*- coding: utf-8 -*-
import unittest
import logging
from ppc_common.ppc_config.sql_storage_config_loader import SQLStorageConfigLoader
from ppc_common.ppc_initialize.dataset_handler_initialize import DataSetHandlerInitialize
from ppc_common.ppc_config.file_chunk_config import FileChunkConfig
from ppc_common.ppc_dataset import dataset_helper_factory
import sys
from ppc_common.ppc_utils import utils


class DataSetInitializerWrapper:
    def __init__(self, ut_obj, file_chunk_size):
        logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
        self.logger = logging.getLogger(__name__)
        self.config = {}
        self.config[SQLStorageConfigLoader.DATABASE_URL_KEY] = "mysql://root:12345678@127.0.0.1:3306/ppc?autocommit=true&charset=utf8mb4"
        self.config[FileChunkConfig.ENABLE_ALL_CHUNCK_FILE_MGR_KEY] = True
        self.config["STORAGE_TYPE"] = "HDFS"
        self.config["HDFS_USER"] = "chenyujie"
        self.config["HDFS_ENDPOINT"] = "http://127.0.0.1:9870"
        self.config[FileChunkConfig.FILE_CHUNK_SIZE_MB_KEY] = file_chunk_size
        self.dataset_handler_initializer = DataSetHandlerInitialize(
            self.config, self.logger)
        self.ut_obj = ut_obj

    def _download_and_check(self, dataset_user, dataset_id, dataset_local_path, expected_md5=None):
       # download
        download_local_path = dataset_local_path + ".download"
        dataset_helper_factory.download_dataset(
            dataset_helper_factory=self.dataset_handler_initializer.dataset_helper_factory,
            dataset_id=dataset_id, dataset_user=dataset_user, logger=self.logger,
            dataset_local_path=download_local_path,
            log_keyword="testDownload", enforce_flush=True
        )
        # check md5
        downloaded_file_md5 = utils.calculate_md5(download_local_path)
        self.ut_obj.assertEquals(expected_md5, downloaded_file_md5)
        # get the dataset
        data = dataset_helper_factory.get_dataset(
            dataset_helper_factory=self.dataset_handler_initializer.dataset_helper_factory,
            dataset_id=dataset_id, dataset_user=dataset_user, logger=self.logger,
            log_keyword="testGetDataSet"
        )
        get_data_md5sum = utils.md5sum(data)
        self.ut_obj.assertEquals(expected_md5, get_data_md5sum)

    def test_dataset_ops(self, dataset_user, dataset_id, dataset_local_path, updated_data):
        # upload the dataset
        dataset_helper_factory.upload_dataset(
            dataset_helper_factory=self.dataset_handler_initializer.dataset_helper_factory,
            dataset_id=dataset_id, dataset_user=dataset_user,
            dataset_local_path=dataset_local_path,
            logger=self.logger, log_keyword="testUpload")
        origin_md5 = utils.calculate_md5(dataset_local_path)
        self._download_and_check(dataset_user=dataset_user,
                                 dataset_id=dataset_id,
                                 dataset_local_path=dataset_local_path,
                                 expected_md5=origin_md5)
        # update dataset
        dataset_helper = self.dataset_handler_initializer.dataset_helper_factory.create(
            dataset_user=dataset_user,
            dataset_id=dataset_id,
            dataset_local_path=None)
        dataset_helper.update_data(updated_data)
        # check
        self._download_and_check(dataset_user=dataset_user,
                                 dataset_id=dataset_id,
                                 dataset_local_path=dataset_local_path,
                                 expected_md5=utils.md5sum(updated_data))
        # rename
        rename_path = dataset_id + ".rename"
        dataset_helper.file_rename(rename_path, True)
        # check
        self._download_and_check(dataset_user=dataset_user,
                                 dataset_id=rename_path,
                                 dataset_local_path=dataset_local_path + ".rename",
                                 expected_md5=utils.md5sum(updated_data))
        # delete
        dataset_helper = dataset_helper_factory.delete_dataset(
            dataset_helper_factory=self.dataset_handler_initializer.dataset_helper_factory,
            dataset_id=rename_path, dataset_user=dataset_user,
            logger=self.logger, log_keyword="testDelete")
        self.ut_obj.assertEquals(dataset_helper.file_object.existed(), False)


class DatasetInitializerTest(unittest.TestCase):
    def testNoFileSplit(self):
        wrapper = DataSetInitializerWrapper(self, 100)
        dataset_user = "yujiechen"
        dataset_id = "test_file_no_split"
        dataset_local_path = "tools/test"
        uploaded_data = "abcdsfsdfsdfssdfsd"
        wrapper.test_dataset_ops(dataset_user=dataset_user,
                                 dataset_id=dataset_id,
                                 dataset_local_path=dataset_local_path,
                                 updated_data=uploaded_data)

    def testFileSplit(self):
        wrapper = DataSetInitializerWrapper(self, 1)
        dataset_user = "yujiechen"
        dataset_id = "test_file_split_file"
        dataset_local_path = "tools/test"
        uploaded_data = "abcdsfsdfsdfssdfsd"
        wrapper.test_dataset_ops(dataset_user=dataset_user,
                                 dataset_id=dataset_id,
                                 dataset_local_path=dataset_local_path,
                                 updated_data=uploaded_data)

    def testFileSplitAndUpdateSplit(self):
        wrapper = DataSetInitializerWrapper(self, 1)
        dataset_user = "yujiechen"
        dataset_id = "test_large_file_split_file"
        dataset_local_path = "tools/test"
        updated_file = "tools/test2"
        uploaded_data = None
        with open(updated_file, "rb") as fp:
            uploaded_data = fp.read()
        wrapper.test_dataset_ops(dataset_user=dataset_user,
                                 dataset_id=dataset_id,
                                 dataset_local_path=dataset_local_path,
                                 updated_data=uploaded_data)


if __name__ == '__main__':
    unittest.main()
