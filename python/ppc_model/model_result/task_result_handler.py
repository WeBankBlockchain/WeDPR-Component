# -*- coding: utf-8 -*-
from ppc_common.ppc_utils.utils import PpcException, PpcErrorCode
from ppc_common.ppc_utils import utils
from ppc_model.common.protocol import ModelTask
from ppc_model.common.base_context import BaseContext
from ppc_common.ppc_ml.model.algorithm_info import ClassificationType
from ppc_model.common.model_result import ResultFileHandling
from ppc_common.ppc_ml.model.algorithm_info import EvaluationType
from ppc_model.common.base_context import BaseContext
from enum import Enum


class TaskResultRequest:
    def __init__(self, job_id, task_type):
        self.job_id = job_id
        self.task_type = task_type


class DataType(Enum):
    TEXT = "str",
    TABLE = "table",
    IMAGE = "image"


class DataItem:
    DEFAULT_NAME_PROPERTY = "metricsName"
    DEFAULT_DATA_PROPERTY = "metricsData"
    DEFAULT_TYPE_PROPERTY = "metricsType"

    def __init__(self, name, data, type,
                 name_property=DEFAULT_NAME_PROPERTY,
                 data_property=DEFAULT_DATA_PROPERTY,
                 type_property=DEFAULT_TYPE_PROPERTY):
        self.name = name
        self.data = data
        self.type = type
        self.name_property = name_property
        self.data_property = data_property
        self.type_property = type_property

    def to_dict(self):
        return {self.name_property: self.name,
                self.data_property: self.data,
                self.type_property: self.type.name}


class ResultFileMeta:
    def __init__(self, table_file_name, retrieve_lines=-1):
        self.table_file_name = table_file_name
        self.retrieve_lines = retrieve_lines


class JobEvaluationResult:
    DEFAULT_TRAIN_EVALUATION_FILES = {
        EvaluationType.ROC: utils.MPC_TRAIN_METRIC_ROC_FILE,
        EvaluationType.PR: utils.MPC_TRAIN_METRIC_PR_FILE,
        EvaluationType.KS: utils.MPC_TRAIN_METRIC_KS_FILE,
        EvaluationType.ACCURACY: utils.MPC_TRAIN_METRIC_ACCURACY_FILE,
        EvaluationType.CONFUSION_MATRIX: utils.MPC_TRAIN_METRIC_CONFUSION_MATRIX_FILE}

    DEFAULT_VALIDATION_EVALUATION_FILES = {
        EvaluationType.ROC: utils.MPC_TRAIN_SET_METRIC_ROC_FILE,
        EvaluationType.PR: utils.MPC_TRAIN_SET_METRIC_PR_FILE,
        EvaluationType.KS: utils.MPC_TRAIN_SET_METRIC_KS_FILE,
        EvaluationType.ACCURACY: utils.MPC_TRAIN_SET_METRIC_ACCURACY_FILE}

    DEFAULT_EVAL_EVALUATION_FILES = {
        EvaluationType.ROC: utils.MPC_EVAL_METRIC_ROC_FILE,
        EvaluationType.PR: utils.MPC_EVAL_METRIC_PR_FILE,
        EvaluationType.KS: utils.MPC_EVAL_METRIC_KS_FILE,
        EvaluationType.ACCURACY: utils.MPC_EVAL_METRIC_ACCURACY_FILE
    }

    def __init__(self, property_name, classification_type,
                 job_id, evaluation_files, components):
        self.job_id = job_id
        self.classification_type = classification_type
        self.components = components
        self.logger = self.components.logger()
        self.classification_type = classification_type
        self.property_name = property_name
        self.evaluation_files = evaluation_files
        self.evaluation_results = []
        try:
            self._fetch_evaluation_result()
            self._fetch_two_classifcation_evaluation_result()
            self._fetch_multi_classifcation_evaluation_result()
        except Exception as e:
            pass

    def _fetch_evaluation_result(self):
        self.logger.info(
            f"fetch roc-evaluation from: {self.evaluation_files[EvaluationType.ROC]}")
        self.evaluation_results.append(DataItem("ROC", ResultFileHandling.make_graph_data(
            self.components,
            self.job_id,
            self.evaluation_files[EvaluationType.ROC]),
            DataType.IMAGE))
        self.logger.info(
            f"fetch pr-evaluation from: {self.evaluation_files[EvaluationType.PR]}")
        self.evaluation_results.append(DataItem("Precision Recall", ResultFileHandling.make_graph_data(
            self.components,
            self.job_id,
            self.evaluation_files[EvaluationType.PR]), DataType.IMAGE))

    def _fetch_two_classifcation_evaluation_result(self):
        if self.classification_type is not ClassificationType.TWO:
            return

        self.logger.info(
            f"fetch ks-evaluation from: {self.evaluation_files[EvaluationType.KS]}")
        self.evaluation_results.append(DataItem("K-S", ResultFileHandling.make_graph_data(
            self.components,
            self.job_id,
            self.evaluation_files[EvaluationType.KS]),
            DataType.IMAGE))

        self.logger.info(
            f"fetch accuracy-evaluation from: {self.evaluation_files[EvaluationType.ACCURACY]}")
        self.evaluation_results.append(DataItem("Accuracy",
                                                ResultFileHandling.make_graph_data(
                                                    self.components,
                                                    self.job_id,
                                                    self.evaluation_files[EvaluationType.ACCURACY]),
                                                DataType.IMAGE))

    def _fetch_multi_classifcation_evaluation_result(self):
        if self.classification_type is not ClassificationType.MULTI:
            return
        self.logger.info(
            f"fetch confusion-matrix-evaluation from: {self.evaluation_files[EvaluationType.CONFUSION_MATRIX]}")
        self.evaluation_results.append(DataItem("Confusion Matrix",
                                                ResultFileHandling.make_graph_data(self.components,
                                                                                   self.job_id,
                                                                                   self.evaluation_files[EvaluationType.CONFUSION_MATRIX]),
                                                DataType.IMAGE))

    def load_ks_table(self, ks_table_file, ks_table_property):
        ks_table_object = TableResult(components=self.components,
                                      job_id=self.job_id, file_meta=ResultFileMeta(table_file_name=ks_table_file))
        self.ks_table = ks_table_object.to_dict()
        self.ks_table_property = ks_table_property

    def to_dict(self):
        evaluation_result_list = []
        for evaluation in self.evaluation_results:
            evaluation_result_list.append(evaluation.to_dict())
        result = {self.property_name: evaluation_result_list}
        if self.ks_table is not None:
            result.update({self.ks_table_property: self.ks_table})
        return result


class TableResult:
    def __init__(self, components, job_id, file_meta):
        self.components = components
        self.job_id = job_id
        self.file_meta = file_meta

    def to_dict(self):
        try:
            df = ResultFileHandling.make_csv_data(self.components, self.job_id,
                                                  self.file_meta.table_file_name)
            csv_columns = list(df.columns)

            if self.file_meta.retrieve_lines == -1 or df.shape[0] <= self.file_meta.retrieve_lines:
                csv_data = df.values.tolist()
            else:
                csv_data = df.iloc[:self.file_meta.retrieve_lines].values.tolist(
                )
            return {'columns': csv_columns, 'data': csv_data}
        except Exception as e:
            pass


class FeatureProcessingResult:
    DEFAULT_FEATURE_PROCESSING_FILES = {
        "PRPreview": ResultFileMeta("xgb_result_column_info_selected.csv"),
        "FEPreview": ResultFileMeta("woe_iv.csv", 5)}

    def __init__(self, components, job_id, file_infos):
        self.components = components
        self.job_id = job_id
        self.file_infos = file_infos
        self.result = dict()
        self._fetch_result()

    def _fetch_result(self):
        for property in self.file_infos.keys():
            table_info = TableResult(self.components,
                                     self.job_id, self.file_infos[property]).to_dict()
            self.result.update({property: table_info})

    def to_dict(self):
        return self.result


class ModelJobResult:
    DEFAULT_PROPERTY_NAME = "outputModelResult"
    MODEL_RESULT = "ModelResult"
    MODEL_RESULT_PATH = "modelResultPath"
    TRAIN_RESULT_PATH = "trainResultPath"
    TEST_RESULT_PATH = "testResultPath"
    WOE_RESULT_PATH = "woeIVResultPath"

    def __init__(self, xgb_job, job_id, components, property_name=DEFAULT_PROPERTY_NAME):
        self.job_id = job_id
        self.xgb_job = xgb_job
        self.components = components
        self.logger = components.logger()
        self.property_name = property_name
        self.model_result_list = None
        self.job_result = None
        self.model_result_path = None
        self.train_result_path = None
        self.woe_iv_result_path = None
        self.model_result_path_dict = None
        self.evaluation_table = None
        self.feature_importance_table = None
        self.iteration_metrics = None

    def fetch_xgb_model_result(self):
        if not self.xgb_job:
            return
        self.model_result_list = []
        i = 0
        # while True:
        while i < 6:
            try:
                tree_data = DataItem(data=ResultFileHandling.make_graph_data(self.components,
                                                                             self.job_id,
                                                                             utils.XGB_TREE_PERFIX + '_' + str(i) + '.svg'),
                                     name='tree-' + str(i), name_property="ModelPlotName", data_property="ModelPlotData",
                                     type=DataType.IMAGE)
                self.model_result_list.append(tree_data.to_dict())
                i += 1
            except Exception:
                break

    def load_result(self, result_path, result_property):
        self.result_property = result_property
        job_result_object = TableResult(self.components,
                                        self.job_id, ResultFileMeta(result_path, 5))
        self.job_result = job_result_object.to_dict()

    def load_model_result_path(self, predict: bool):
        self.model_result_path_dict = dict()
        self.model_result_path = ResultFileHandling.get_remote_path(
            self.components, self.job_id, BaseContext.MODEL_DATA_FILE)
        self.model_result_path_dict.update(
            {ModelJobResult.MODEL_RESULT_PATH: self.model_result_path})

        self.train_result_path = ResultFileHandling.get_remote_path(
            self.components, self.job_id, BaseContext.TRAIN_MODEL_OUTPUT_FILE)
        self.model_result_path_dict.update(
            {ModelJobResult.TRAIN_RESULT_PATH: self.train_result_path})

        self.model_result_path_dict.update(
            {ModelJobResult.TEST_RESULT_PATH: ResultFileHandling.get_remote_path(
                self.components, self.job_id, BaseContext.TEST_MODEL_OUTPUT_FILE)})

        self.woe_iv_result_path = ResultFileHandling.get_remote_path(
            self.components, self.job_id, BaseContext.WOE_IV_FILE)
        self.model_result_path_dict.update(
            {ModelJobResult.WOE_RESULT_PATH: self.woe_iv_result_path})

    def load_evaluation_table(self, evaluation_path, property):
        evaluation_table_object = TableResult(self.components,
                                              self.job_id, ResultFileMeta(evaluation_path))
        self.evaluation_table = {property: DataItem(name=property, data=evaluation_table_object.to_dict(),
                                                    type=DataType.TABLE).to_dict()}

    def load_feature_importance_table(self, feature_importance_path, property):
        if not self.xgb_job:
            return
        feature_importance_table = TableResult(self.components,
                                               self.job_id, ResultFileMeta(feature_importance_path))
        self.feature_importance_table = {property: DataItem(name=property, data=feature_importance_table.to_dict(),
                                                            type=DataType.TABLE).to_dict()}

    def load_iteration_metrics(self, iteration_path, property):
        if not self.xgb_job:
            return
        try:
            iteration_metrics_data = DataItem(data=ResultFileHandling.make_graph_data(self.components, self.job_id, utils.METRICS_OVER_ITERATION_FILE),
                                              name='iteration_metrics', name_property="ModelPlotName", data_property="ModelPlotData",
                                              type=DataType.IMAGE)
            self.iteration_metrics = []
            self.iteration_property = property
            self.iteration_metrics.append(iteration_metrics_data.to_dict())
        except:
            pass

    def to_dict(self):
        result = dict()
        if self.model_result_list is not None:
            result.update({self.property_name: self.model_result_list})
        if self.job_result is not None:
            result.update({self.result_property: self.job_result})
        if self.evaluation_table is not None:
            result.update(self.evaluation_table)
        if self.feature_importance_table is not None:
            result.update(self.feature_importance_table)
        if self.iteration_metrics is not None:
            result.update({self.iteration_property: self.iteration_metrics})
        if self.model_result_path_dict is not None:
            result.update(
                {ModelJobResult.MODEL_RESULT: self.model_result_path_dict})
        return result


class TaskResultHandler:
    def __init__(self, task_result_request: TaskResultRequest, components):
        self.task_result_request = task_result_request
        self.components = components
        self.logger = components.logger()
        self.result_list = []
        self.predict = False
        self.xgb_job = False
        if self.task_result_request.task_type == ModelTask.XGB_PREDICTING.name or self.task_result_request.task_type == ModelTask.LR_PREDICTING.name:
            self.predict = True
        if self.task_result_request.task_type == ModelTask.XGB_PREDICTING.name or self.task_result_request.task_type == ModelTask.XGB_TRAINING.name:
            self.xgb_job = True
        self.logger.info(
            f"Init jobResultHandler for: {self.task_result_request.job_id}")
        self._get_evaluation_result()
        self._get_feature_processing_result()

    def get_response(self):
        merged_result = dict()
        for result in self.result_list:
            merged_result.update(result.to_dict())
        response = {"jobPlanetResult":  merged_result}
        return utils.make_response(PpcErrorCode.SUCCESS.get_code(), PpcErrorCode.SUCCESS.get_msg(), response)

    def _get_evaluation_result(self):
        if not self.predict:
            # the train evaluation result
            self.train_evaluation_result = JobEvaluationResult(
                property_name="outputMetricsGraphs",
                classification_type=ClassificationType.TWO,
                job_id=self.task_result_request.job_id,
                evaluation_files=JobEvaluationResult.DEFAULT_TRAIN_EVALUATION_FILES,
                components=self.components)
            # load the ks table
            self.train_evaluation_result.load_ks_table(
                "mpc_train_metric_ks.csv", "TrainKSTable")
            self.result_list.append(self.train_evaluation_result)

            self.validation_evaluation_result = JobEvaluationResult(
                property_name="outputTrainMetricsGraphs",
                classification_type=ClassificationType.TWO,
                job_id=self.task_result_request.job_id,
                evaluation_files=JobEvaluationResult.DEFAULT_VALIDATION_EVALUATION_FILES,
                components=self.components)
            # load the ks_table
            self.validation_evaluation_result.load_ks_table(
                "mpc_metric_ks.csv", "KSTable")
            self.result_list.append(self.validation_evaluation_result)

            self.model = ModelJobResult(self.xgb_job,
                                        self.task_result_request.job_id, self.components, ModelJobResult.DEFAULT_PROPERTY_NAME)
            self.model.fetch_xgb_model_result()
            # the ks-auc table
            self.model.load_evaluation_table(
                utils.MPC_XGB_EVALUATION_TABLE, "EvaluationTable")
            # the feature-importance table
            self.model.load_feature_importance_table(
                utils.XGB_FEATURE_IMPORTANCE_TABLE, "FeatureImportance")
            self.result_list.append(self.model)
            # the metrics iteration graph
            self.model.load_iteration_metrics(
                utils.METRICS_OVER_ITERATION_FILE, "IterationGraph")

        if self.predict:
            # the train evaluation result
            self.predict_evaluation_result = JobEvaluationResult(
                property_name="outputMetricsGraphs",
                classification_type=ClassificationType.TWO,
                job_id=self.task_result_request.job_id,
                evaluation_files=JobEvaluationResult.DEFAULT_EVAL_EVALUATION_FILES,
                components=self.components)
            # load ks_table
            self.predict_evaluation_result.load_ks_table(
                "mpc_eval_metric_ks.csv", "KSTable")
            self.result_list.append(self.predict_evaluation_result)

        # load model_result
        self.model_result = ModelJobResult(self.xgb_job,
                                           self.task_result_request.job_id, self.components, ModelJobResult.DEFAULT_PROPERTY_NAME)
        self.model_result.load_result(
            BaseContext.TRAIN_MODEL_OUTPUT_FILE, "outputTrainPreview")
        self.model_result.load_model_result_path(self.predict)
        self.result_list.append(self.model_result)

    def _get_feature_processing_result(self):
        self.feature_processing_result = FeatureProcessingResult(
            self.components, self.task_result_request.job_id, FeatureProcessingResult.DEFAULT_FEATURE_PROCESSING_FILES)
        self.result_list.append(self.feature_processing_result)
