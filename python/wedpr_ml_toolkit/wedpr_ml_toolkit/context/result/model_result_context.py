from wedpr_ml_toolkit.context.result.result_context import ResultContext
from wedpr_ml_toolkit.context.result.fe_result_context import PreprocessingResultContext
from wedpr_ml_toolkit.context.result.fe_result_context import FeResultContext
from wedpr_ml_toolkit.context.job_context import JobContext
from wedpr_ml_toolkit.transport.wedpr_remote_job_client import JobDetailResponse
from wedpr_ml_toolkit.context.dataset_context import DatasetContext


class ModelResultContext(PreprocessingResultContext, FeResultContext):
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        super().__init__(job_context, job_result_detail)
        PreprocessingResultContext.__init__(
            self, job_context, job_result_detail)
        FeResultContext.__init__(self, job_context, job_result_detail)


class TrainResultContext(ModelResultContext):
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        self.train_result_dataset: DatasetContext = None
        self.test_result_dataset: DatasetContext = None
        self.model_result_dataset: DatasetContext = None
        self.model_dataset: DatasetContext = None
        self.evaluation_dataset: DatasetContext = None
        self.feature_selection_dataset: DatasetContext = None
        self.feature_importance_dataset: DatasetContext = None
        super().__init__(job_context, job_result_detail)

    def __repr__(self):
        return f"train_result_dataset: {self.train_result_dataset}," \
               f"test_result_dataset: {self.test_result_dataset}," \
               f"model_result_dataset: {self.model_result_dataset}," \
               f"model_dataset: {self.model_dataset}," \
               f"evaluation_dataset: {self.evaluation_dataset}," \
               f"feature_selection_dataset: {self.feature_selection_dataset}," \
               f"feature_importance_dataset: {self.feature_importance_dataset}," \
               f"preprocessing_dataset: {self.preprocessing_dataset}," \
               f"fe_dataset: {self.fe_dataset}," \
               f"psi_dataset: {self.psi_result_file_path}"

    def parse_result(self):
        # train_model_output
        self.train_result_dataset = self._generate_result_dataset_(
            self.job_result_detail.modelResultDetail['ModelResult']['trainResultPath'])
        #  test_model_output
        self.test_result_dataset = self._generate_result_dataset_(
            self.job_result_detail.modelResultDetail['ModelResult']['testResultPath'])
        # xgb_tree.json
        self.model_result_dataset = self._generate_result_dataset_(
            self.model_result_path)
        # model_enc.kpl
        self.model_dataset = self._generate_result_dataset_(self.model_path)
        # mpc_xgb_evaluation_table.csv
        self.evaluation_dataset = self._generate_result_dataset_(
            self.evaluation_table_path)
        # xgb_result_column_info_selected.csv
        self.feature_selection_dataset = self._generate_result_dataset_(
            self.feature_selection_file_path)
        # xgb_result_feature_importance_table.csv
        self.feature_importance_dataset = self._generate_result_dataset_(
            self.feature_importance_result_path)


class PredictResultContext(ResultContext):
    def __init__(self, job_context: JobContext, job_result_detail: JobDetailResponse):
        self.model_result_dataset: DatasetContext = None
        super().__init__(job_context, job_result_detail)

    def __repr__(self):
        return f"model_result_dataset: {self.model_result_dataset}"

    def parse_result(self):
        self.model_result_dataset = self._generate_result_dataset_(
            self.job_result_detail.modelResultDetail['ModelResult']['testResultPath'])
