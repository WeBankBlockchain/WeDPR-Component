# -*- coding: utf-8 -*-

class Constant:
    NUMERIC_ARRAY = [i for i in range(10)]
    HTTP_STATUS_OK = 200
    WEDPR_API_PREFIX = '/api/wedpr/v3/'
    DEFAULT_SUBMIT_JOB_URI = f'{WEDPR_API_PREFIX}project/submitJob'
    DEFAULT_QUERY_JOB_STATUS_URL = f'{WEDPR_API_PREFIX}project/queryJobByCondition'
    DEFAULT_QUERY_JOB_DETAIL_URL = f'{WEDPR_API_PREFIX}scheduler/queryJobDetail'
    PSI_RESULT_FILE = "psi_result.csv"

    FEATURE_BIN_FILE = "feature_bin.json"
    TEST_MODEL_OUTPUT_FILE = "test_output.csv"
    TRAIN_MODEL_OUTPUT_FILE = "train_output.csv"

    FE_RESULT_FILE = "fe_result.csv"
