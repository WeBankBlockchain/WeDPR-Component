import numpy as np
import pandas as pd

from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.common.context import Context


class FeatureBinning:
    def __init__(self, ctx: Context):
        self.ctx = ctx
        self.params = ctx.model_params
        self.data = None
        self.data_bin = None
        self.data_split = None

    @staticmethod
    def binning_continuous_feature(feature: np.ndarray, max_bin: int, is_equal_freq: bool = True):
        try:
            if is_equal_freq:
                # 等频分箱，不替换缺失值
                Xk_bin, Xk_split = pd.qcut(
                    feature, q=max_bin, retbins=True, labels=False)
            else:
                # 等距分箱，不替换缺失值
                Xk_bin, Xk_split = pd.cut(
                    feature, max_bin, retbins=True, labels=False)
            # 将缺失值标记为 -1
            Xk_bin[np.isnan(feature)] = -1
        except ValueError:
            unique_values = sorted(set(feature[~np.isnan(feature)]))
            if len(unique_values) > 1000:
                raise Exception(
                    'Features with more than 1000 groups are not supported.')
            if len(unique_values) == 2 and 0 in unique_values and 1 in unique_values:
                Xk_bin = feature
                Xk_split = [min(unique_values) - 0.01, 0.5, max(unique_values)]
            elif len(unique_values) > max_bin:
                Xk_bin, Xk_split = pd.cut(
                    feature, max_bin, labels=False, retbins=True)
            else:
                # 创建映射字典
                mapping_dict = {value: i for i,
                                value in enumerate(unique_values)}
                # 使用map函数进行重新编号
                Xk_bin = pd.DataFrame(feature)[0].map(mapping_dict).values
                Xk_split = [min(unique_values) - 0.01] + list(unique_values)
            Xk_bin[np.isnan(feature)] = -1

        return Xk_bin, Xk_split

    @staticmethod
    def binning_categorical_feature(feature: np.ndarray):
        unique_values = sorted(set(feature[~np.isnan(feature)]))
        if len(unique_values) > 1000:
            raise Exception(
                'Features with more than 1000 groups are not supported.')
        mapping_dict = {value: i for i, value in enumerate(unique_values)}
        # 使用map函数进行重新编号
        Xk_bin = pd.DataFrame(feature)[0].map(mapping_dict).values
        Xk_split = [min(unique_values) - 0.01] + list(unique_values)
        Xk_bin[np.isnan(feature)] = -1

        return Xk_bin, Xk_split

    def data_binning(self, data: np.ndarray, data_split=None):

        self.data = data
        self.data_split = data_split

        if self.ctx.algorithm_type == AlgorithmType.Train.name and self.data_split is None:
            self._generate_data_binning()
        else:
            self._reuse_data_binning(data_split)

        return self.data_bin, self.data_split

    def _generate_data_binning(self):

        n = self.data.shape[0]
        d = self.data.shape[1]

        X_bin = np.zeros((d, n), dtype='int16')
        X_split = []

        for idx, feature in enumerate(self.data.T):
            if idx in self.params.my_categorical_idx:
                Xk_bin, Xk_split = FeatureBinning.binning_categorical_feature(
                    feature)
            else:
                Xk_bin, Xk_split = FeatureBinning.binning_continuous_feature(
                    feature, self.params.max_bin)

            X_bin[idx] = Xk_bin
            if isinstance(Xk_split, np.ndarray):
                Xk_split = Xk_split.tolist()
            X_split.append(Xk_split)

        self.data_bin = X_bin.T
        self.data_split = X_split

    def _reuse_data_binning(self, data_split):

        self.data_split = data_split

        n = self.data.shape[0]
        d = self.data.shape[1]

        test_X_bin = np.zeros((d, n), dtype='int16')

        for k in range(d):
            bin_min = min(self.data[:, k]) - 1
            bin_max = max(self.data[:, k]) + 1
            if np.isnan(bin_min):
                bin_min = min(self.data_split[k])
            if np.isnan(bin_max):
                bin_max = max(self.data_split[k])

            bin_min = min(bin_min, min(self.data_split[k])) - 1
            bin_max = max(bin_max, max(self.data_split[k])) + 1

            if len(self.data_split[k]) > 2:
                bins = np.concatenate(
                    ([bin_min], self.data_split[k][1:-1], [bin_max]), axis=0)
            else:
                bins = np.concatenate(([bin_min], [bin_max]), axis=0)
            test_Xk_bin = pd.cut(self.data[:, k], bins, labels=False)
            test_Xk_bin[np.isnan(test_Xk_bin)] = -1

            test_X_bin[k] = test_Xk_bin

        self.data_bin = test_X_bin.T
