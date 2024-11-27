# -*- coding: utf-8 -*-

class Constant:
    NUMERIC_ARRAY = [i for i in range(10)]
    HTTP_STATUS_OK = 200
    WEDPR_API_PREFIX = 'api/wedpr/v3/'
    DEFAULT_SUBMIT_JOB_URI = f'{WEDPR_API_PREFIX}project/submitJob'
    DEFAULT_QUERY_JOB_STATUS_URL = f'{WEDPR_API_PREFIX}project/queryJobByCondition'
    DEFAULT_QUERY_JOB_DETAIL_URL = f'{WEDPR_API_PREFIX}scheduler/queryJobDetail'
    # the dataset related url
    DEFAULT_QUERY_DATASET_URL = f'{WEDPR_API_PREFIX}dataset/queryDataset'
    DEFAULT_UPDATED_DATASET_URL = f'{WEDPR_API_PREFIX}dataset/updateDatasetMeta'
    PSI_RESULT_FILE = "psi_result.csv"

    FEATURE_BIN_FILE = "feature_bin.json"
    XGB_TREE_PREFIX = "xgb_tree"
    MODEL_RESULT_FILE = XGB_TREE_PREFIX + '.json'
    PREPROCESSING_RESULT_FILE = "preprocessing_result.csv"
    EVALUATION_TABLE_FILE = "mpc_xgb_evaluation_table.csv"
    FEATURE_IMPORTANCE_FILE = "xgb_result_feature_importance_table.csv"
    FEATURE_SELECTION_FILE = "xgb_result_column_info_selected.csv"
    MODEL_FILE = "model_enc.kpl"
    WOE_IV_FILE = "woe_iv.csv"
