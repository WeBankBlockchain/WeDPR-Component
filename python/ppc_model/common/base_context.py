import os
from ppc_common.ppc_utils import utils


class BaseContext:
    PSI_RESULT_FILE = "psi_result.csv"
    MODEL_PREPARE_FILE = "model_prepare.csv"
    PREPROCESSING_RESULT_FILE = "preprocessing_result.csv"
    EVAL_COLUMN_FILE = "model_eval_column.csv"
    WOE_IV_FILE = 'woe_iv.csv'
    IV_SELECTED_FILE = 'iv_selected.csv'
    SELECTED_COL_FILE = "xgb_result_column_info_selected.csv"

    # TODO: rename xgb filename
    FEATURE_BIN_FILE = "feature_bin.json"
    # MODEL_DATA_FILE = "model_data.json"
    MODEL_DATA_FILE = utils.XGB_TREE_PERFIX + '.json'
    TEST_MODEL_RESULT_FILE = "model_result.csv"
    # TEST_MODEL_OUTPUT_FILE = "model_output.csv"
    TEST_MODEL_OUTPUT_FILE = "xgb_output.csv"
    TRAIN_MODEL_RESULT_FILE = "train_model_result.csv"
    # TRAIN_MODEL_OUTPUT_FILE = "train_model_output.csv"
    TRAIN_MODEL_OUTPUT_FILE = "xgb_train_output.csv"

    def __init__(self, job_id: str, job_temp_dir: str):
        self.job_id = job_id
        self.workspace = os.path.join(job_temp_dir, self.job_id)
        if not os.path.exists(self.workspace):
            os.makedirs(self.workspace)
        self.psi_result_path = os.path.join(self.workspace, self.PSI_RESULT_FILE)
        self.model_prepare_file = os.path.join(self.workspace, self.MODEL_PREPARE_FILE)
        self.preprocessing_result_file = os.path.join(self.workspace, self.PREPROCESSING_RESULT_FILE)
        self.eval_column_file = os.path.join(self.workspace, self.EVAL_COLUMN_FILE)
        self.woe_iv_file = os.path.join(self.workspace, self.WOE_IV_FILE)
        self.iv_selected_file = os.path.join(self.workspace, self.IV_SELECTED_FILE)
        self.selected_col_file = os.path.join(self.workspace, self.SELECTED_COL_FILE)
        self.remote_selected_col_file = os.path.join(self.job_id, self.SELECTED_COL_FILE)

        self.summary_evaluation_file = os.path.join(self.workspace, utils.MPC_XGB_EVALUATION_TABLE)
        self.feature_importance_file = os.path.join(self.workspace, utils.XGB_FEATURE_IMPORTANCE_TABLE)
        self.feature_bin_file = os.path.join(self.workspace, self.FEATURE_BIN_FILE)
        self.model_data_file = os.path.join(self.workspace, self.MODEL_DATA_FILE)
        self.test_model_result_file = os.path.join(self.workspace, self.TEST_MODEL_RESULT_FILE)
        self.test_model_output_file = os.path.join(self.workspace, self.TEST_MODEL_OUTPUT_FILE)
        self.train_model_result_file = os.path.join(self.workspace, self.TRAIN_MODEL_RESULT_FILE)
        self.train_model_output_file = os.path.join(self.workspace, self.TRAIN_MODEL_OUTPUT_FILE)

        self.train_metric_roc_file = os.path.join(self.workspace, utils.MPC_TRAIN_SET_METRIC_ROC_FILE)
        self.train_metric_ks_file = os.path.join(self.workspace, utils.MPC_TRAIN_SET_METRIC_KS_FILE)
        self.train_metric_pr_file = os.path.join(self.workspace, utils.MPC_TRAIN_SET_METRIC_PR_FILE)
        self.train_metric_acc_file = os.path.join(self.workspace, utils.MPC_TRAIN_SET_METRIC_ACCURACY_FILE)
        self.test_metric_roc_file = os.path.join(self.workspace, utils.MPC_TRAIN_METRIC_ROC_FILE)
        self.test_metric_ks_file = os.path.join(self.workspace, utils.MPC_TRAIN_METRIC_KS_FILE)
        self.test_metric_pr_file = os.path.join(self.workspace, utils.MPC_TRAIN_METRIC_PR_FILE)
        self.test_metric_acc_file = os.path.join(self.workspace, utils.MPC_TRAIN_METRIC_ACCURACY_FILE)
        self.train_metric_ks_table = os.path.join(self.workspace, utils.MPC_TRAIN_SET_METRIC_KS_TABLE)
        self.test_metric_ks_table = os.path.join(self.workspace, utils.MPC_TRAIN_METRIC_KS_TABLE)
        self.model_tree_prefix = os.path.join(self.workspace, utils.XGB_TREE_PERFIX)
        self.metrics_iteration_file = os.path.join(self.workspace, utils.METRICS_OVER_ITERATION_FILE)

        self.remote_summary_evaluation_file = os.path.join(self.job_id, utils.MPC_XGB_EVALUATION_TABLE)
        self.remote_feature_importance_file = os.path.join(self.job_id, utils.XGB_FEATURE_IMPORTANCE_TABLE)
        self.remote_feature_bin_file = os.path.join(self.job_id, self.FEATURE_BIN_FILE)
        self.remote_model_data_file = os.path.join(self.job_id, self.MODEL_DATA_FILE)
        self.remote_test_model_output_file = os.path.join(self.job_id, self.TEST_MODEL_OUTPUT_FILE)
        self.remote_train_model_output_file = os.path.join(self.job_id, self.TRAIN_MODEL_OUTPUT_FILE)

        self.remote_train_metric_roc_file = os.path.join(self.job_id, utils.MPC_TRAIN_SET_METRIC_ROC_FILE)
        self.remote_train_metric_ks_file = os.path.join(self.job_id, utils.MPC_TRAIN_SET_METRIC_KS_FILE)
        self.remote_train_metric_pr_file = os.path.join(self.job_id, utils.MPC_TRAIN_SET_METRIC_PR_FILE)
        self.remote_train_metric_acc_file = os.path.join(self.job_id, utils.MPC_TRAIN_SET_METRIC_ACCURACY_FILE)
        self.remote_test_metric_roc_file = os.path.join(self.job_id, utils.MPC_TRAIN_METRIC_ROC_FILE)
        self.remote_test_metric_ks_file = os.path.join(self.job_id, utils.MPC_TRAIN_METRIC_KS_FILE)
        self.remote_test_metric_pr_file = os.path.join(self.job_id, utils.MPC_TRAIN_METRIC_PR_FILE)
        self.remote_test_metric_acc_file = os.path.join(self.job_id, utils.MPC_TRAIN_METRIC_ACCURACY_FILE)
        self.remote_train_metric_ks_table = os.path.join(self.job_id, utils.MPC_TRAIN_SET_METRIC_KS_TABLE)
        self.remote_test_metric_ks_table = os.path.join(self.job_id, utils.MPC_TRAIN_METRIC_KS_TABLE)
        self.remote_model_tree_prefix = os.path.join(self.job_id, utils.XGB_TREE_PERFIX)
        self.remote_metrics_iteration_file = os.path.join(self.job_id, utils.METRICS_OVER_ITERATION_FILE)

    @staticmethod
    def feature_engineering_input_path(job_id: str, job_temp_dir: str):
        return os.path.join(job_temp_dir, job_id, BaseContext.MODEL_PREPARE_FILE)
