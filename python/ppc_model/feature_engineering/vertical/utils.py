import numpy as np
import pandas as pd

from ppc_model.datasets.feature_binning.feature_binning import FeatureBinning


def is_continuous_feature(categorical: str, field):
    if categorical == '0':
        return True
    return field not in categorical.split(',')


def calculate_woe_iv_with_pos_event(grouped):
    grouped['neg_event'] = grouped['count'] - grouped['pos_event']

    # 避免出现无穷大woe
    grouped['pos_event'] = grouped['pos_event'].astype(np.float64)
    grouped['neg_event'] = grouped['neg_event'].astype(np.float64)
    grouped.loc[grouped['neg_event'] == 0, 'pos_event'] += 0.5
    grouped.loc[grouped['neg_event'] == 0, 'neg_event'] = 0.5
    grouped.loc[grouped['pos_event'] == 0, 'neg_event'] += 0.5
    grouped.loc[grouped['pos_event'] == 0, 'pos_event'] = 0.5

    # 计算WOE和IV值
    grouped['pos_event_rate'] = grouped['pos_event'] / \
        (grouped['pos_event'].sum())
    grouped['neg_event_rate'] = grouped['neg_event'] / \
        (grouped['neg_event'].sum())
    grouped['woe'] = np.log(grouped['pos_event_rate'] /
                            (grouped['neg_event_rate']))
    grouped['iv'] = (grouped['pos_event_rate'] -
                     grouped['neg_event_rate']) * grouped['woe']
    iv_total = grouped['iv'].sum()
    grouped['iv_total'] = iv_total
    return grouped, iv_total


def calculate_woe_iv(feature: np.ndarray, label: np.ndarray, num_bins: int = 10, is_continuous: bool = True,
                     is_equal_freq: bool = True):
    # 将特征和目标变量合并
    combined = pd.DataFrame({'feature': feature, 'label': label})
    # 按特征值对数据集进行分箱
    if is_continuous:
        combined['bins'] = FeatureBinning.binning_continuous_feature(
            feature, num_bins, is_equal_freq)[0]
    else:
        combined['bins'] = FeatureBinning.binning_categorical_feature(feature)[
            0]
    # 计算每个分箱中的正负样本数量和总体样本数量
    grouped = combined.groupby('bins')['label'].agg(['count', 'sum'])
    grouped = grouped.rename(columns={'sum': 'pos_event'})

    return calculate_woe_iv_with_pos_event(grouped)


def calculate_woe_iv_with_pos_event(grouped):
    grouped['neg_event'] = grouped['count'] - grouped['pos_event']

    # 避免出现无穷大woe
    grouped['pos_event'] = grouped['pos_event'].astype(np.float64)
    grouped['neg_event'] = grouped['neg_event'].astype(np.float64)
    grouped.loc[grouped['neg_event'] == 0, 'pos_event'] += 0.5
    grouped.loc[grouped['neg_event'] == 0, 'neg_event'] = 0.5
    grouped.loc[grouped['pos_event'] == 0, 'neg_event'] += 0.5
    grouped.loc[grouped['pos_event'] == 0, 'pos_event'] = 0.5

    # 计算WOE和IV值
    grouped['pos_event_rate'] = grouped['pos_event'] / \
        (grouped['pos_event'].sum())
    grouped['neg_event_rate'] = grouped['neg_event'] / \
        (grouped['neg_event'].sum())
    grouped['woe'] = np.log(grouped['pos_event_rate'] /
                            (grouped['neg_event_rate']))
    grouped['iv'] = (grouped['pos_event_rate'] -
                     grouped['neg_event_rate']) * grouped['woe']
    iv_total = grouped['iv'].sum()
    grouped['iv_total'] = iv_total
    return grouped, iv_total
