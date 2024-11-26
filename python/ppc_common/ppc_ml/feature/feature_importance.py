# -*- coding: utf-8 -*-
from enum import Enum
import pandas as pd
from ppc_common.deps_services.serialize_type import SerializeType


class FeatureGainInfo:
    def __init__(self, feature_idx, feature_gain):
        self.feature_idx = feature_idx
        self.feature_gain = feature_gain

# TODO: cover


class FeatureImportanceType(Enum):
    WEIGHT = 'weight'  # The number of times the feature is used in all trees
    GAIN = 'gain'  # The gain of features in predictions across all trees

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_


class FeatureImportance:
    """the feature importance
    """

    def __init__(self, importance_type: FeatureImportanceType, importance=0):
        self.importance_type = importance_type
        self.importance = importance

    def inc(self, value):
        self.importance += value

    def desc(self):
        return f"importance type: {self._importance_type}, importance: {self.importance}"

    def __cmp__(self, other):
        if self.importance > other.importance:
            return 1
        elif self.importance < other.importance:
            return -1
        return 0

    def __eq__(self, other):
        return self.importance == other.importance

    def __lt__(self, other):
        return self.importance < other.importance

    def __add__(self, other):
        new_importance = FeatureImportance(
            self.importance_type, importance=self.importance + other.importance)
        return new_importance


class ReadOnlyFeatureImportanceStore:
    RANK_PROPERTY = "score_rank"
    DEAULT_SCORE_THRESHOLD = 0.95
    DEFAULT_TOPK_PROPERTY = "topk"
    DEFAULT_EMPTY_TOPK_FLAG = " "
    DEFAULT_TOPK_FLAG = "True"

    def __init__(self, feature_importance_dict, logger):
        # feature_importance_type ==> fid ==> value
        self.feature_importance_dict = feature_importance_dict
        self.logger = logger

    def get_feature_importance(self, fid, type):
        if type not in self.feature_importance_dict:
            return 0
        if fid not in self.feature_importance_dict[type]:
            return 0
        return self.feature_importance_dict[type][fid].importance

    def _get_sorting_columns(self):
        sorting_columns = []
        ascending_list = []
        if FeatureImportanceType.GAIN in self.feature_importance_type_list:
            sorting_columns.append(FeatureImportanceType.GAIN.name)
            ascending_list.append(False)
        if FeatureImportanceType.WEIGHT in self.feature_importance_type_list:
            sorting_columns.append(FeatureImportanceType.WEIGHT.name)
            ascending_list.append(False)
        return (sorting_columns, ascending_list)

    @staticmethod
    def load(df, logger):
        """load the feature importance

        Args:
            df (DataFrame): the feature importance
        """
        logger.debug(f"load feature_importance data: {df}")
        feature_importance_type_list = []
        for column in df.columns:
            if not FeatureImportanceType.has_value(column.lower()):
                continue
            enum_feature_importance_type = FeatureImportanceType(
                column.lower())
            feature_importance_type_list.append(enum_feature_importance_type)
        logger.debug(
            f"load feature_importance, feature_importance_type_list: {feature_importance_type_list}")
        # feature_importance_type ==> fid_key ==> value
        feature_importance_dict = dict()
        for row in df.itertuples():
            fid_key = getattr(
                row, FeatureImportanceStore.DEFAULT_FEATURE_PROPERTY)
            for importance_type in feature_importance_type_list:
                value = getattr(row, importance_type.name)
                if importance_type not in feature_importance_dict:
                    feature_importance_dict.update({importance_type: dict()})
                feature_importance_dict[importance_type].update(
                    {fid_key: FeatureImportance(importance_type, value)})
        return ReadOnlyFeatureImportanceStore(feature_importance_dict, logger)

    def to_dataframe(self, topk_threshold=0.95):
        """convert the feature importance into pd
        the format:
        | feature | score   | score_rank | topk|
        | x16 | 0.08234 |   1 | |
        | x11 | 0.08134 |   2 | |
        | x1  | 0.08034 |   3 | |
        """
        if self.feature_importance_dict is None or len(self.feature_importance_dict) < 1:
            return None
        df = pd.DataFrame()
        # the feature column
        df.insert(df.shape[1], FeatureImportanceStore.DEFAULT_FEATURE_PROPERTY,
                  self.feature_importance_dict[self.feature_importance_type_list[0]].keys())
        # the importance columns
        for importance_type in self.feature_importance_dict.keys():
            feature_importance_values = []
            for feature_importance in self.feature_importance_dict[importance_type].values():
                feature_importance_values.append(feature_importance.importance)
            # calculate weight-average for the score
            if importance_type == FeatureImportanceType.GAIN:
                feature_importance_sum = sum(feature_importance_values)
                if feature_importance_sum != 0:
                    for i in range(len(feature_importance_values)):
                        feature_importance_values[i] = float(
                            feature_importance_values[i]) / feature_importance_sum

            df.insert(df.shape[1], importance_type.name,
                      feature_importance_values)
        # sort by the importance
        (sorting_columns, ascending_list) = self._get_sorting_columns()
        df = df.sort_values(by=sorting_columns, ascending=ascending_list)
        # rank
        df.insert(df.shape[1],
                  ReadOnlyFeatureImportanceStore.RANK_PROPERTY,
                  range(1, len(df) + 1))
        # top-k
        if FeatureImportanceType.GAIN not in self.feature_importance_type_list:
            return df
        score_sum = float(0)
        topk_list = [
            ReadOnlyFeatureImportanceStore.DEFAULT_EMPTY_TOPK_FLAG for _ in range(len(df))]
        i = 0
        for score in df[FeatureImportanceType.GAIN.name].T:
            score_sum += score
            if score_sum >= topk_threshold:
                topk_list[i] = ReadOnlyFeatureImportanceStore.DEFAULT_TOPK_FLAG
                break
            i += 1
        df.insert(
            df.shape[1], ReadOnlyFeatureImportanceStore.DEFAULT_TOPK_PROPERTY, topk_list)
        return df


class FeatureImportanceStore(ReadOnlyFeatureImportanceStore):
    """store all the feature importance
    """
    DEFAULT_FEATURE_PROPERTY = "feature"
    DEFAULT_IMPORTANCE_LIST = [
        FeatureImportanceType.GAIN, FeatureImportanceType.WEIGHT]

    def __init__(self, feature_importance_type_list, feature_list, logger):
        # feature_importance_type ==> fid ==> value
        super().__init__(dict(), logger)
        self.feature_list = feature_list
        if self.feature_list is not None:
            self.logger.info(
                f"create FeatureImportanceStore, all features: {self.feature_list}")
        self.feature_importance_type_list = feature_importance_type_list
        for importance_type in self.feature_importance_type_list:
            self.feature_importance_dict.update({importance_type: dict()})
        self._init()

    def _init(self):
        if self.feature_list is None:
            return
        for i in range(len(self.feature_list)):
            gain_list = {FeatureImportanceType.GAIN: 0,
                         FeatureImportanceType.WEIGHT: 0}
            self.update_feature_importance(i, gain_list)

    def set_init(self, feature_list):
        self.feature_list = feature_list
        self._init()

    def generate_fid_key(self, fid):
        if fid >= len(self.feature_list):
            return None
        return f"{self.feature_list[fid]}"

    def update_feature_importance(self, fid, gain_list):
        """update the feature importance

        Args:
            fid (int): the idx of the best feature(maxk)
            gain_list (dict): the gain list for every importance_type

        Raises:
            Exception: invalid gain_list
        """
        fid_key = self.generate_fid_key(fid)
        if fid_key is None:
            return
        for importance_type in gain_list.keys():
            # unknown importance_type
            if importance_type not in self.feature_importance_dict:
                continue
            if fid_key not in self.feature_importance_dict[importance_type]:
                self.feature_importance_dict[importance_type][fid_key] = FeatureImportance(
                    importance_type, 0)
            self.feature_importance_dict[importance_type][fid_key].inc(
                gain_list[importance_type])

    def store(self, serialize_type: SerializeType, local_file_path, remote_file_path, storage_client, user=None):
        """store the feature importance into file, upload using storage_client

        Args:
            storage_client: the client used to upload the result
        """
        df = self.to_dataframe()
        if serialize_type is SerializeType.CSV:
            df.to_csv(local_file_path, index=False)
        else:
            df.to_json(orient='split', path_or_buf=local_file_path)
        self.logger.info(
            f"Store feature_importance to {local_file_path}, file type: {serialize_type}")
        if storage_client is not None:
            storage_client.upload_file(local_file_path, remote_file_path, user)
            self.logger.info(
                f"Upload feature_importance to {local_file_path} success, file type: {serialize_type}")
