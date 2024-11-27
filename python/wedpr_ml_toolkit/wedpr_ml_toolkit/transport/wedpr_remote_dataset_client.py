# -*- coding: utf-8 -*-
from wedpr_ml_toolkit.common.utils.base_object import BaseObject
from wedpr_ml_toolkit.common.utils.base_object import WeDPRResponse
from wedpr_ml_toolkit.transport.wedpr_entrypoint import WeDPREntryPoint
from wedpr_ml_toolkit.config.wedpr_ml_config import DatasetConfig
from wedpr_ml_toolkit.config.wedpr_ml_config import HttpConfig
from wedpr_ml_toolkit.config.wedpr_ml_config import AuthConfig
from wedpr_ml_toolkit.config.wedpr_ml_config import UserConfig
from typing import Any
import json


class DatasetMeta(BaseObject):
    def __init__(self, dataset_id: str = None,
                 dataset_label: str = None,
                 dataset_title: str = None,
                 dataset_desc: str = None,
                 dataset_fields: str = None,
                 dataset_hash: str = None,
                 dataset_size: int = -1,
                 dataset_record_count: int = -1,
                 dataset_column_count: int = -1,
                 dataset_storage_type: str = None,
                 dataset_storage_path: str = None,
                 datasource_type: str = None,
                 datasource_meta: str = None,
                 agency: str = None,
                 user: str = None,
                 status: str = None,
                 status_desc: str = None,
                 file_path: str = None,
                 user_config: UserConfig = None,
                 **params: Any):
        self.datasetId = dataset_id
        self.datasetLabel = dataset_label
        self.datasetTitle = dataset_title
        self.datasetDesc = dataset_desc
        self.datasetFields = dataset_fields
        self.datasetHash = dataset_hash
        self.datasetSize = dataset_size
        self.recordCount = dataset_record_count
        self.columnCount = dataset_column_count
        self.datasetStorageType = dataset_storage_type
        self.datasetStoragePath = dataset_storage_path
        self.dataSourceType = datasource_type
        self.dataSourceMeta = datasource_meta
        self.ownerAgencyName = agency
        self.ownerUserName = user
        self.user_config = user_config
        if self.user_config is not None:
            self.ownerAgencyName = self.user_config.agency_name
            self.ownerUserName = self.user_config.user
        self.status = status
        self.statusDesc = status_desc
        self.file_path = file_path
        self.set_params(**params)
        if self.datasetStoragePath is not None:
            dataset_storage_path_dict = json.loads(self.datasetStoragePath)
            if "filePath" in dataset_storage_path_dict.keys():
                self.file_path = dataset_storage_path_dict.get("filePath")

    def __repr__(self):
        return f"id: {self.datasetId}, " \
               f"file_path, {self.file_path}, " \
               f"datasetTitle: {self.datasetTitle}, datasetFields: {self.datasetFields}, " \
               f"datasetSize: {self.datasetSize}, recordCount: {self.recordCount}," \
               f"columnCount: {self.columnCount}, datasetStorageType: {self.datasetStorageType}, " \
               f"user: {self.ownerUserName}, " \
               f"agency: {self.ownerAgencyName}"


class WeDPRDatasetClient(WeDPREntryPoint, BaseObject):
    def __init__(self, http_config: HttpConfig, auth_config: AuthConfig, dataset_config: DatasetConfig):
        if auth_config is None:
            raise Exception("Must define the auth config!")
        if http_config is None:
            raise Exception("Must define the http config")
        super().__init__(auth_config.access_key_id, auth_config.access_key_secret,
                         auth_config.get_remote_entrypoints_list(), http_config, auth_config.nonce_len)
        self.auth_config = auth_config
        self.http_config = http_config
        self.dataset_config = dataset_config

    def query_dataset(self, dataset_id) -> DatasetMeta:
        params = {}
        params.update({"datasetId": dataset_id})
        response_dict = self.send_request(
            False, self.dataset_config.query_dataset_uri, params, None, None)
        wedpr_response = WeDPRResponse(**response_dict)
        # query the dataset failed
        if wedpr_response.success() is False:
            raise Exception(
                f"Query dataset information failed for {wedpr_response.msg}, dataset_id: {dataset_id}")
        # query success, deserialize to DatasetMeta
        return DatasetMeta(**wedpr_response.data)

    def update_dataset(self, dataset_meta: DatasetMeta):
        if dataset_meta.datasetId is None or len(dataset_meta.datasetId) == 0:
            raise Exception(
                f"Invalid dataset meta {dataset_meta}, must define the datasetId")
        response_dict = self.send_request(is_post=True,
                                          uri=self.dataset_config.update_dataset_uri,
                                          headers=None,
                                          params=None,
                                          data=json.dumps(dataset_meta.as_dict()))
        wedpr_response = WeDPRResponse(**response_dict)
        if wedpr_response.success() is False:
            raise Exception(
                f"Update dataset meta {dataset_meta} failed, msg: {wedpr_response.msg}")
