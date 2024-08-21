# -*- coding: utf-8 -*-
import unittest
from ppc_common.ppc_utils import utils
import os
import time


class FileOperationWrapper:
    def __init__(self, file_chunk_dir, merged_file_path):
        self.file_chunk_dir = file_chunk_dir
        self.merged_file_path = merged_file_path
        chunk_list = os.listdir(self.file_chunk_dir)
        self.file_chunk_list = []
        for chunk in chunk_list:
            self.file_chunk_list.append(os.path.join(file_chunk_dir, chunk))


class TestFileOperations(unittest.TestCase):
    def test_merge_and_calculate_hash_for_files(self):
        chunk_file_dir = "/data/app/files/ppcs-modeladm/dataset"
        # chunk_file_dir = "bak/object/train_test.csv"
        wrapper = FileOperationWrapper(chunk_file_dir, "dataset1")
        start_t = time.time()
        print(
            f"#### begin merge file for {len(wrapper.file_chunk_list)} chunks")
        utils.merge_files(file_list=wrapper.file_chunk_list,
                          output_file=wrapper.merged_file_path)
        print(
            f"#### success merge file for {len(wrapper.file_chunk_list)} chunks success, time cost: {time.time() - start_t}")

        print(
            f"#### calculate hash for {wrapper.merged_file_path}, size: {os.stat(wrapper.merged_file_path).st_size}")
        start_t = time.time()
        utils.calculate_md5(wrapper.merged_file_path)
        print(
            f"#### calculate hash for {wrapper.merged_file_path} success, size: {os.stat(wrapper.merged_file_path).st_size}, timecost: {time.time() - start_t}")


if __name__ == '__main__':
    unittest.main()
