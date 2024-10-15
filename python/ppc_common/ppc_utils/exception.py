# -*- coding: utf-8 -*-
from enum import Enum, unique


@unique
class PpcErrorCode(Enum):
    SUCCESS = {0: 'success'}
    INTERNAL_ERROR = {10000: "internal error"}

    NETWORK_ERROR = {10001: 'network error'}
    JOB_STATUS_ERROR = {10002: 'job status check error'}
    JOB_ROLE_ERROR = {10003: 'job role check error'}
    DATABASE_ERROR = {10004: 'database related operation error'}
    DATASET_CSV_ERROR = {10005: 'dataset csv format error'}
    DATASET_PATH_ERROR = {10006: 'dataset path permission check error'}
    PARAMETER_CHECK_ERROR = {10007: 'parameter check error'}
    CALL_SYNCS_SERVICE_ERROR = {10008: 'call syncs service error'}
    DATA_SET_ERROR = {10009: 'dataset operation error'}

    INSUFFICIENT_AUTHORITY = {10010: 'insufficient authority'}
    UNDEFINED_TYPE = {10011: 'undefined type'}
    UNDEFINED_STATUS = {10012: 'undefined status'}
    QUERY_USERNAME_ERROR = {10013: 'query username error'}
    DATASET_NOT_FOUND = {10014: 'dataset not found'}
    ALGORITHM_NOT_FOUND = {10015: 'algorithm queried not found'}
    JOB_NOT_FOUND = {10016: 'job not found'}
    AUTH_INFO_FOUND = {10017: 'authorization not found'}
    AGENCY_NOT_FOUND = {10018: 'agency not found'}
    AGENCY_MANAGEMENT_NOT_FOUND = {10019: 'agency management not found'}
    FIELD_NOT_FOUND = {10020: 'dataset field not found'}
    AUTH_ALREADY_EXISTED = {10021: 'authorization already existed'}
    DATABASE_TYPE_ERROR = {10022: 'database type error'}
    DATABASE_IP_ERROR = {10023: 'database ip not in the white list'}
    DATASET_FROM_DB_RETRY_OVER_LIMIT_ERROR = {
        10024: 'access the database has exceeded the allowed limit times'}

    DATASET_PARSE_ERROR = {10300: 'parse dataset failed'}
    DATASET_EXIST_ERROR = {10301: 'dataset already existed!'}
    DATASET_DELETE_ERROR = {10302: 'dataset already deleted!'}
    DATASET_PERMISSION_ERROR = {10303: 'dataset permission check failed!'}
    DATASET_UPLOAD_ERROR = {10304: 'dataset upload error'}

    ALGORITHM_PARSE_ERROR = {10400: 'parse algorithm failed'}
    ALGORITHM_EXIST_ERROR = {10401: 'algorithm already existed!'}
    ALGORITHM_DELETE_ERROR = {10402: 'algorithm already deleted!'}
    ALGORITHM_TYPE_ERROR = {10403: 'algorithm type is not existed!'}
    ALGORITHM_COMPILE_ERROR = {10410: 'compile mpc algorithm error'}
    ALGORITHM_BAD_SQL = {10411: 'bad sql'}
    ALGORITHM_PPC_CONFIG_ERROR = {10412: 'parse algorithm config error'}
    ALGORITHM_PPC_MODEL_ALGORITHM_NAME_ERROR = {
        10413: 'algorithm subtype not found'}
    ALGORITHM_PPC_MODEL_OUTPUT_NUMBER_ERROR = {10414: 'output number error'}
    ALGORITHM_PPC_MODEL_LAYERS_ERROR = {
        10415: 'layers attribute should not be set'}
    ALGORITHM_MPC_SYNTAX_CHECK_ERROR = {10416: 'check ppc mpc syntax error'}
    ALGORITHM_PPC_MODEL_OUTPUT_NUMBER_ERROR_TEMP = {
        10417: 'output number should be set 1'}
    ALGORITHM_PPC_MODEL_PARTICIPANTS_ERROR_TEMP = {
        10418: 'participants should be greater or equal to 2'}
    ALGORITHM_PPC_MODEL_TEST_DATASET_PERCENTAGE_ERROR = {
        10419: 'test_dataset_percentage should be set in (0, 0.5]'}
    ALGORITHM_PPC_MODEL_EPOCHS_ERROR = {
        10420: 'epochs should be set in [1, 5]'}
    ALGORITHM_PPC_MODEL_BATCH_SIZE_ERROR = {
        10421: 'batch_size should be set [1, min(128, max_train_dataset_size)]'}
    ALGORITHM_PPC_MODEL_THREADS_ERROR = {
        10422: 'threads should be set in [1,8]'}
    ALGORITHM_PPC_MODEL_OPTIMIZER_ERROR = {10423: 'optimizer not found'}
    ALGORITHM_PPC_MODEL_LEARNING_RATE_ERROR = {
        10424: 'learning rate should be set in (0, 1)'}
    ALGORITHM_PPC_MODEL_LAYERS_ERROR2 = {
        10425: 'Conv2d layer should not be the first layer in HeteroNN'}
    ALGORITHM_NOT_EXIST_ERROR = {10426: 'algorithm does not exist!'}
    ALGORITHM_PPC_MODEL_TREES_ERROR = {
        10427: 'num_trees should be set in [1, 300]'}
    ALGORITHM_PPC_MODEL_DEPTH_ERROR = {
        10428: 'max_depth should be set in [1, 10]'}

    JOB_CREATE_ERROR = {10500: 'job create failed'}
    JOB_COMPUTATION_EXISTED_ERROR = {10501: 'job computation not existed'}
    JOB_AYS_MODE_CHECK_ERROR = {10502: 'patch request need static token'}
    JOB_MANAGEMENT_RUN_ERROR = {10503: 'job run failed'}
    JOB_CEM_ERROR = {10504: 'at least one field need to be provided'}
    JOB_IS_RUNNING_ERROR = {10505: 'job is running'}
    NO_PARTICAPATING_IN_JOB_ERROR = {10506: 'not participating in the job'}
    JOB_DOWNLOAD_RESULT_EMPTY = {10507: '任务结果为空，无法下载'}

    HDFS_STORAGE_ERROR = {10601: 'hdfs storage error'}

    AYS_LENGTH_ERROR = {10701: 'base ot message length check error'}
    AYS_ORDER_ERROR = {10702: 'search obfuscation order must > 1'}
    AYS_RESULT_LENGTH_ERROR = {10703: 'message result length check error'}
    AYS_FIELD_ERROR = {10704: 'search filed not found in obfuscate file'}
    CALL_SCS_ERROR = {10705: 'computation node call error'}

    MERGE_FILE_CHECK_ERROR = {10801: 'merge check files failed'}
    MERGE_FILE_FORMAT_ERROR = {10802: 'file format is not csv'}
    FILE_SIZE_ERROR = {10803: 'cannot get file size'}
    FILE_SPLIT_ERROR = {10804: 'split file failed!'}
    FILE_NOT_EXIST_ERROR = {10805: 'share file not existed!'}
    DUPLICATED_MERGE_FILE_REQUEST = {10806: 'duplicated merge file request'}

    XGB_PREPROCESSING_ERROR = {10901: 'xgb preprocessing failed!'}

    FILE_OBJECT_UPLOAD_CHECK_FAILED = {20000: "upload file object failed!"}
    FILE_OBJECT_NOT_EXISTS = {20001: "the file not exists!"}

    TASK_EXISTS = {11000: "the task already exists!"}
    TASK_NOT_FOUND = {11001: "the task not found!"}
    TASK_IS_KILLED = {11002: "the task is killed!"}

    ROLE_TYPE_ERROR = {12000: "role type is illegal."}
    
    UNSUPPORTED_WORK_TYPE = {13000: "unsupported work type."}

    def get_code(self):
        return list(self.value.keys())[0]

    def get_error_code(self):
        return list(self.value.keys())[0]

    def get_msg(self):
        return list(self.value.values())[0]

    def get_message(self):
        return list(self.value.values())[0]


class PpcException(Exception):

    def __init__(self, code, message):
        Exception.__init__(self)
        self.code = code
        self.message = message

    def to_dict(self):
        return {'code': self.code, 'message': self.message}

    def get_code(self):
        return self.code

    def __str__(self):
        return self.message

    @classmethod
    def by_ppc_error_code(cls, ppc_error_code):
        cls.code = ppc_error_code.get_code()
        cls.message = ppc_error_code.get_msg()
