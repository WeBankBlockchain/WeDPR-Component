# -*- coding: utf-8 -*-
# from concurrent.futures import ProcessPoolExecutor, as_completed

import json
import os

import numpy as np
import pandas as pd
from sklearn.preprocessing import MinMaxScaler, StandardScaler
from ppc_model.common.model_setting import ModelSetting

from ppc_common.ppc_utils import utils
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_model.common.global_context import components
from ppc_model.preprocessing.local_processing.psi_select import calculate_psi
from ppc_model.preprocessing.local_processing.standard_type_enum import standardType
# from ppc_common.ppc_mock.mock_objects import MockLogger

# components.mock_logger = MockLogger()
# log = components.mock_logger
log = components.logger()


def process_train_dataframe(dataset_df: pd.DataFrame, column_info_dict: dict):
    """
    使用column_info对dataset_df进行处理 只保留column_info中的列

    参数:
    - dataset_df: 待处理的DataFrame数据
    - column_info: 字段信息, 字典类型

    返回值:
    - dataset_df_filled: 处理后的DataFrame数据
    """
    # dataset_df_filled = None
    # Iterate over column_info_dict keys, if 'isExisted' is False, drop the column
    for key, value in column_info_dict.items():
        if value.get('isExisted') is None:
            raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
            ), "column_info_dict isExisted is None")
        if value.get('isExisted') is False:
            dataset_df = dataset_df.drop(key, axis=1)

    return dataset_df


def process_dataframe(dataset_df: pd.DataFrame, model_setting: ModelSetting, xgb_data_file_path: str,
                      ppc_job_type: str = utils.AlgorithmType.Train.name, job_id: str = None, ctx=None):
    """
    对数据集进行预处理的函数。
    共执行6步操作
    1. 去除唯一属性: 为了让无意义的参数不影响模型, 比如id, id不代表样本本身的规律, 所以要把这些属性删掉
    2. 缺失值处理: 删除含有缺失值的特征,若变量的缺失率较高(大于80%)，覆盖率较低，且重要性较低，可以直接将变量删除.
    每一行的数据不一定拥有所有的模型标签，支持不填充，和均值插补, 
    我们目前使用均值插补，以该属性存在值的平均值来插补缺失的值
    连续性：普通均值
    类别：单独类别，当做新类别
    3. 离群值处理方法: 数据过大或过小会一个峡谷分析结果，要先调整因子值的离群值上下限，减少离群值的影响，防止下一步归一化后的偏差过大
    我们用的3 \sigma 法，又叫做标准差法
    4. 特征编码: 将特征编码为固定的值 比如one-hot就是将不同的type 编码为1 2 3 4 5...这样的值
    5. 数据标准化: min-max标准化(归一化):最大值1. 最小值0或-1 以及z-score标准化 规范化:均值为0 标准差为1
    6. 特征选择: 从给定的特征集合中选出相关特征子集的过程称为特征选择, 我们使用的是PSI 风控模型，群体稳定性指标 PSI-Population Stability Index
    参考 https://zhuanlan.zhihu.com/p/79682292


    参数：
    dataset_df (pandas.DataFrame): 输入的数据集。
    xgb_data_file_path (str): XGBoost数据文件路径。

    返回：
    column_info: 处理后的数据集字段。
    """
    log.info(
        f"jobid: {job_id}, xgb_data_file_path:{xgb_data_file_path}, ppc_job_type: {ppc_job_type}")
    if model_setting is None:
        log.error("model_setting is None")
        raise PpcException(
            PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(), "model_setting is None")

    column_info = {}

    if ppc_job_type != utils.AlgorithmType.Predict.name:
        # 如果是训练任务 先默认所有数据都存在
        column_info = {col: {'isExisted': True} for col in dataset_df.columns}

        if model_setting.eval_set_column is not None:
            if model_setting.eval_set_column in dataset_df.columns:
                eval_column = model_setting.eval_set_column
                dataset_df[['id', eval_column]].to_csv(
                    ctx.eval_column_file, index=None)
                ctx.components.storage_client.upload_file(
                    ctx.eval_column_file, job_id + os.sep + ctx.EVAL_COLUMN_FILE)
                if model_setting.eval_set_column != model_setting.psi_select_col:
                    dataset_df = dataset_df.drop(columns=[eval_column])

    categorical_cols = ['id', 'y']

    # 判断 model_setting['categorical']是否为None
    if model_setting.categorical is None:
        log.error(
            f"jobid: {job_id} model_setting['categorical'] is None, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
        ), "xgb_model_dict['categorical'] is None")

    if model_setting.fillna is None:
        log.error(
            f"jobid: {job_id} xgb_model_dict['fillna'] is None, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
        ), "xgb_model_dict['fillna'] is None")

    # 指定分类特征索引
    if model_setting.categorical != '0':
        categoricals = model_setting.categorical.split(',')
        categorical_cols.extend(categoricals)
        # 去除categorical_cols中的重复元素
        categorical_cols = list(set(categorical_cols))

    df_filled = dataset_df
    # 预处理表格信息 包含每一列缺失值比例 缺失值是否被筛掉 psi筛选和相关性筛选 如果在某个阶段被筛掉则设置为0 保留则为1
    # 1.去除唯一属性
    if 'id' in df_filled.columns:
        log.info(f"jobid: {job_id} move id column start.")
        df_filled = df_filled.drop('id', axis=1)
        log.info(f"jobid: {job_id} move id column finish.")

    # 2.1 缺失值筛选
    if ppc_job_type != utils.AlgorithmType.Predict.name:
        if 0 <= model_setting.na_select <= 1:
            log.info(f"jobid: {job_id} run fillna start")
            df_filled, column_info = process_na_dataframe(
                df_filled, model_setting.na_select)
            log.info(f"jobid: {job_id} run fillna finish")
        else:
            log.error(
                f"jobid: {job_id} xgb_model_dict['na_select'] is range not 0 to 1, xgb_model_dict:{model_setting}")
            raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                               "xgb_model_dict['na_select'] range not 0 to 1")
    else:
        log.info(f"jobid: {job_id} don't need run fillna for predict job.")

    # 2.2 缺失值处理
    if model_setting.fillna == 1:
        # 填充
        log.info(f"jobid: {job_id} run fillna with means start")
        try:
            df_filled = process_na_fill_dataframe(
                df_filled, categorical_cols, model_setting.psi_select_col)
            log.info(f"jobid: {job_id} run fillna with means finish")
        except Exception as e:
            log.error(
                f"jobid: {job_id} process_na_fill_dataframe error, e:{e}")
            raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
            ), "process_na_fill_dataframe error")
    elif model_setting.fillna == 0:
        # 不填充
        log.info(f"jobid: {job_id} don't need run fillna ")
        # 如果本身是None就不需要处理
        df_filled.replace('None', np.nan)
    else:
        log.error(
            f"jobid: {job_id} xgb_model_dict['fillna'] is not 0 or 1, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
        ), "xgb_model_dict['fillna'] is not 0 or 1")

    # 6.1 特征选择 进行 psi稳定性指标筛选 计算特征相关性 降维可以减少模型的复杂度，提高模型的泛化能力
    if ppc_job_type != utils.AlgorithmType.Predict.name:
        if model_setting.psi_select_col in df_filled.columns.tolist() and model_setting.psi_select_col != 0:
            log.info(f"jobid: {job_id} run psi_select_col start")
            psi_select_base = model_setting.psi_select_base
            psi_select_thresh = model_setting.psi_select_thresh
            psi_select_bins = model_setting.psi_select_bins
            psi_select_col = model_setting.psi_select_col

            if psi_select_base is None or psi_select_thresh is None or psi_select_bins is None:
                log.error(
                    f"jobid: {job_id} psi_select_base or psi_select_thresh or psi_select_bins is None, model_setting:{model_setting}")
                raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                                   "psi_select_base or psi_select_thresh or psi_select_bins is None")
            df_filled, psi_selected_cols = process_psi(df_filled, categorical_cols, psi_select_col, psi_select_base,
                                                       psi_select_thresh, psi_select_bins)
            # 使用column_info和psi_selected_cols 追加psi_selected列 如果列还在则选中是1 否则是0
            for col in column_info.keys():
                if col in psi_selected_cols:
                    column_info[col]['psi_selected'] = 1
                    column_info[col]['isExisted'] = True
                else:
                    column_info[col]['psi_selected'] = 0
                    column_info[col]['isExisted'] = False
            log.info(f"jobid: {job_id} run psi_select_col finish")
        elif model_setting.psi_select_col == 0 or model_setting.psi_select_col == "":
            log.info(f"jobid: {job_id} don't need run psi_select_col")
        else:
            log.error(
                f"jobid: {job_id} xgb_model_dict['psi_select_col'] is not 0 or in col, model_setting:{model_setting}")
            raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                               "xgb_model_dict['psi_select_col'] is not 0 or in col")
    else:
        log.info(
            f"jobid: {job_id} don't need run psi_select_col for predict job.")

    # 6.2 特征选择 进行 corr_select 计算特征相关性
    if ppc_job_type != utils.AlgorithmType.Predict.name:
        if model_setting.corr_select > 0:
            log.info(f"jobid: {job_id} run corr_select start")
            corr_select = model_setting.corr_select
            df_filled = remove_high_correlation_features(
                df_filled, categorical_cols, corr_select)
            # 设置相关性筛选的column_info
            for col in column_info.keys():
                if col in df_filled.columns.tolist():
                    column_info[col]['corr_selected'] = 1
                    column_info[col]['isExisted'] = True
                else:
                    column_info[col]['corr_selected'] = 0
                    column_info[col]['isExisted'] = False
            log.info(f"jobid: {job_id} run corr_select finish")
        elif model_setting.corr_select == 0:
            log.info(f"jobid: {job_id} don't need run corr_select")
        else:
            log.error(
                f"jobid: {job_id} xgb_model_dict['corr_select'] is not >= 0, model_setting:{model_setting}")
            raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                               "xgb_model_dict['corr_select'] is not >= 0")
    else:
        log.info(
            f"jobid: {job_id} don't need run corr_select for predict job.")

    # 3. 离群值处理 3-sigma 法
    if model_setting.filloutlier == 1:
        log.info(f"jobid: {job_id} run filloutlier start")
        df_filled = process_outliers(df_filled, categorical_cols)
        log.info(f"jobid: {job_id} run filloutlier finish")
    elif model_setting.filloutlier == 0:
        log.info(f"jobid: {job_id} don't need run filloutlier")
    else:
        log.error(
            f"jobid: {job_id} xgb_model_dict['filloutlier'] is not 0 or 1, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                           "xgb_model_dict['filloutlier'] is not 0 or 1")

    # 5.1 数据标准化 支持max-min normalized
    if model_setting.normalized == 1:
        log.info(f"jobid: {job_id} run normalized start")
        df_filled = normalize_dataframe(
            df_filled, categorical_cols, standardType.min_max.value)
        log.info(f"jobid: {job_id} run normalized finish")
    elif model_setting.normalized == 0:
        log.info(f"jobid: {job_id} don't need run normalized")
    else:
        log.error(
            f"jobid: {job_id} xgb_model_dict['normalized'] is not 0 or 1, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                           "xgb_model_dict['normalized'] is not 0 or 1")

    # 5.2 z-score标准化（规范化） standardized
    if model_setting.standardized == 1:
        log.info(f"jobid: {job_id} run standardized start")
        df_filled = normalize_dataframe(
            df_filled, categorical_cols, standardType.z_score.value)
        log.info(f"jobid: {job_id} run standardized finish")
    elif model_setting.standardized == 0:
        log.info(f"jobid: {job_id} don't need run standardized")
    else:
        log.error(
            f"jobid: {job_id} xgb_model_dict['standardized'] is not 0 or 1, model_setting:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                           "xgb_model_dict['standardized'] is not 0 or 1")

    # 4. 特征编码，对分类特征进行one-hot编码 这里会多一些列 所以放到最后
    if model_setting.one_hot == 1:
        log.info(f"jobid: {job_id} run one_hot start")
        df_filled = one_hot_encode_and_merge(df_filled, categorical_cols)
        log.info(f"jobid: {job_id} run one_hot finish")
    elif model_setting.one_hot == 0:
        log.info(f"jobid: {job_id} don't need run one_hot")
    else:
        log.error(
            f"jobid: {job_id} model_setting['one_hot'] is not 0 or 1, xgb_model_dict:{model_setting}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
        ), "model_setting['one_hot'] is not 0 or 1")
    df_filled.to_csv(xgb_data_file_path, mode='w',
                     sep=utils.BLANK_SEP, header=True, index=None)
    # log.info(f"jobid: {job_id} column_info:{column_info} in type: {ppc_job_type}, process_dataframe succeed")
    log.info(
        f"jobid: {job_id} in type: {ppc_job_type}, process_dataframe succeed")
    return column_info


def process_na_dataframe(df: pd.DataFrame, na_select: float):
    """
    缺失值处理 如果小于阈值则移除该列

    参数
    - df: 待处理的DataFrame数据
    - na_select: 缺失值占比阈值

    返回值：
    处理后的DataFrame数据
    """
    missing_ratios = df.isnull().mean()

    # 剔除缺失值占比大于na_select的列
    selected_cols = missing_ratios[missing_ratios <= na_select].index

    column_info = {col: {'missing_ratio': missing_ratios[col], 'na_selected': 1 if col in selected_cols else 0, } for
                   col in df.columns}
    # 如果没被选择 将isExisted设置为false
    for col in column_info.keys():
        if col not in selected_cols:
            column_info[col]['isExisted'] = False
        else:
            column_info[col]['isExisted'] = True
    df_processed = df[selected_cols]
    return df_processed, column_info


def process_na_fill_dataframe(df: pd.DataFrame, categorical_cols: list = None, psi_select_col: str = None):
    """
    处理DataFrame数据:
    1. 计算每一列的缺失值占比；
    2. 剔除缺失值占比大于阈值的列；
    3. 使用每列的均值填充剩余的缺失值。

    参数：
    - df: 待处理的DataFrame数据
    - na_select: 缺失值占比阈值
    - categorical_cols: 分类特征列


    返回值：
    处理后的DataFrame数据
    """
    # 计算每一列的缺失值占比

    # 判断categorical_cols是否在df中，如果是，填充则用col的max+1, 否则用均值插补
    df_processed = df.copy()  # Assign the sliced DataFrame to a new variable
    for col in df_processed.columns.to_list():
        # 如果col是y，则忽略
        if col == 'y':
            continue
        elif psi_select_col and col == psi_select_col:
            continue
        if col in categorical_cols:
            df_processed.fillna(
                {col: df_processed[col].max() + 1}, inplace=True)
        else:
            df_processed.fillna({col: df_processed[col].mean()}, inplace=True)
    return df_processed


def process_outliers(df: pd.DataFrame, categorical_cols: list):
    """
    处理DataFrame数据中的异常值 
    1. 计算每一列的均值和标准差；
    2. 对于超出均值+-3倍标准差的数据 使用均值填充。

    参数：
    - df: 待处理的DataFrame数据
    - categorical_cols: 列表，包含分类列的名称

    返回值：
    处理后的DataFrame数据
    """
    # 计算每一列的均值和标准差
    means = df.mean()
    threshold = 3 * df.std()

    # 定义处理异常值的函数
    def replace_outliers(col):
        # 如果列是分类列，不处理
        if col.name in categorical_cols:
            return col
        else:
            lower_bound = means[col.name] - threshold[col.name]
            upper_bound = means[col.name] + threshold[col.name]
            # 如果元素是空值或不是异常值，保持不变；否则使用均值填充
            return np.where(col.notna() & ((col < lower_bound) | (col > upper_bound)), means[col.name], col)

    # 应用处理异常值的函数到DataFrame的每一列
    df_processed = df.apply(replace_outliers)
    return df_processed


def one_hot_encode_and_merge(df_filled: pd.DataFrame, categorical_features: list):
    """
    对DataFrame中指定的分类特征列进行One-Hot编码 并将编码后的结果合并到DataFrame中。

    参数：
    - df: 待处理的DataFrame数据
    - categorical_features: 分类特征列表 需要进行One-Hot编码的列名

    返回值：
    处理后的DataFrame数据
    """
    categorical_cols_without_y_and_id = categorical_features.copy()
    if 'id' in categorical_cols_without_y_and_id:
        categorical_cols_without_y_and_id.remove('id')
    if 'y' in categorical_cols_without_y_and_id:
        categorical_cols_without_y_and_id.remove('y')
    # 如果categorical_cols_without_y_and_id不在df_filled的col name中 报错
    if not set(categorical_cols_without_y_and_id).issubset(set(df_filled.columns)):
        log.error(
            f"categorical_cols_without_y_and_id is not in df_filled columns, categorical_cols_without_y_and_id:{categorical_cols_without_y_and_id}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(),
                           "categorical_cols_without_y_and_id is not in df_filled columns")
    df_merged = pd.get_dummies(
        df_filled, columns=categorical_cols_without_y_and_id)

    return df_merged


def normalize_dataframe(df: pd.DataFrame, categorical_cols: list, type: standardType = standardType.min_max.value):
    """
    对DataFrame进行标准化，排除指定的分类特征

    参数：
    - df: 待标准化的DataFrame数据
    - categorical_cols: 列表，包含不需要标准化的分类特征的名称

    返回值：
    标准化后的DataFrame数据
    """
    # 创建MinMaxScaler对象
    if type == standardType.min_max.value:
        scaler = MinMaxScaler()
    elif type == standardType.z_score.value:
        scaler = StandardScaler()
    else:
        log.error(f"unspport type in normalize_dataframe type:{type}")
        raise PpcException(PpcErrorCode.XGB_PREPROCESSING_ERROR.get_code(
        ), "unspport type in normalize_dataframe type")

    # 获取数值变量的列名
    numeric_cols = df.select_dtypes(include=['int', 'float']).columns

    # 排除指定的分类特征
    numeric_cols = [col for col in numeric_cols if col not in categorical_cols]

    # 对数值变量进行标准化
    df_normalized = df.copy()
    df_normalized[numeric_cols] = scaler.fit_transform(
        df_normalized[numeric_cols])
    return df_normalized

# def calculate_correlation(df_i_col_name, df_j_col_name, i, j, col_i, col_j, categorical_cols):
#     if df_i_col_name in categorical_cols or df_j_col_name in categorical_cols:
#         return None  # 返回NaN表示忽略这个计算
#     common_index = col_i.index.intersection(col_j.index)
#     return i,j,np.corrcoef(col_i.loc[common_index], col_j.loc[common_index])[0, 1]


def remove_high_correlation_features(df: pd.DataFrame, categorical_cols: list, corr_select: float):
    """
    删除DataFrame中相关系数大于corr_select的特征中的一个。

    参数：
    - df: 待处理的DataFrame数据
    - categorical_cols: 列表，包含分类特征的名称
    - corr_select: 相关系数阈值

    返回值：
    处理后的DataFrame数据
    """
    # ===========原有逻辑==============
    # 计算特征之间的相关系数矩阵
    # num_features = df.shape[1]
    # correlation_matrix = np.zeros((num_features, num_features))

    # for i in range(num_features):
    #     for j in range(i+1, num_features):
    #         # if i == j:
    #         #     continue
    #         # 忽略categorical_cols的列
    #         if df.columns[i] in categorical_cols or df.columns[j] in categorical_cols:
    #             continue
    #         # 当有缺失值时 去除该行对应的缺失值进行比较, 比如col1有缺失值, col2没有, 那么col1的缺失值对应的位置，和col2一起去掉，共同参与计算
    #         col_i = df.iloc[:, i].dropna()
    #         col_j = df.iloc[:, j].dropna()
    #         common_index = col_i.index.intersection(col_j.index)
    #         correlation_matrix[i, j] = np.corrcoef(col_i.loc[common_index], col_j.loc[common_index])[0, 1]
    #     # print(f"correlation_matrix: {correlation_matrix}")
    #         # correlation_matrix[i, j] = np.corrcoef(df.iloc[:, i], df.iloc[:, j])[0, 1]

    # # 获取相关系数大于corr_select的特征对, 获取列名
    # high_correlation = np.argwhere(np.abs(correlation_matrix) > corr_select)

    # high_correlation_col_name = []
    # for i, j in high_correlation:
    #     high_correlation_col_name.append((df.columns[i], df.columns[j]))
    # # 删除相关性大于corr_select的特征中的一个
    # for col_left, col_right in high_correlation_col_name:
    #     try:
    #         # 如果col_left和col_right都在df中, 删除右边, 否则不动
    #         if col_left in df.columns and col_right in df.columns:
    #             df.drop(col_right, axis=1, inplace=True)
    #         # df.drop(df.columns[j], axis=1, inplace=True)
    #     except:
    #         log.warning(
    #             f"remove_high_correlation_features error, i:{i}, j:{j}, df[col_left]:{df[col_left]}, df[col_right]:{df[col_right]}")
    #         pass
    # return df
    # ===========原有逻辑==============
    num_features = df.shape[1]
    correlation_matrix = np.zeros((num_features, num_features))

    df_copy = df.copy()
    for col_name in categorical_cols:
        if col_name in df_copy.columns:
            df_copy.drop(col_name, axis=1, inplace=True)
    correlation_matrix = df_copy.corr()
    # ===========尝试已提交多线程==============
    # 提前设置好需要的列
    # col_list = {}
    # for i in range(num_features):
    #     for j in range(i+1, num_features):
    #         col_list[i,j]=df.iloc[:, i].dropna().copy(), df.iloc[:, j].dropna().copy()
    # print(correlation_matrix)
    # for i in range(num_features):
    #     for j in range(i+1, num_features):
    #         result = calculate_correlation(df.columns[i], df.columns[j], i, j, df.iloc[:, i], df.iloc[:, j], categorical_cols)
    #         if result != None:
    #             i,j, correlation_matrix_i_j = result
    #             correlation_matrix[i,j] = correlation_matrix_i_j
    # with ProcessPoolExecutor() as executor:
    #     col_i, col_j = col_list[i,j]
    #     results = [executor.submit(calculate_correlation, df.columns[i], df.columns[j], i, j, col_i, col_j, categorical_cols) for j in range(i + 1, num_features)]
    # for future in as_completed(results):
    #     result = future.result()
    #     # print(f"result: {result}, {type(result)}")
    #     if result != None:
    #         i,j, correlation_matrix_i_j = result
    #         correlation_matrix[i,j] = correlation_matrix_i_j
    # print(f"correlation_matrix: {correlation_matrix}")
    # ===========尝试已提交多线程==============
    high_correlation = np.argwhere(np.abs(correlation_matrix) > corr_select)

    high_correlation_col_name = []
    for i, j in high_correlation:
        if i >= j:
            continue
        high_correlation_col_name.append(
            (df_copy.columns[i], df_copy.columns[j]))
    # 删除相关性大于corr_select的特征中的一个
    for col_left, col_right in high_correlation_col_name:
        try:
            # 如果col_left和col_right都在df中, 删除右边, 否则不动
            if col_left in df.columns and col_right in df.columns:
                df.drop(col_right, axis=1, inplace=True)
            # df.drop(df.columns[j], axis=1, inplace=True)
        except:
            log.warn(
                f"remove_high_correlation_features error, i:{i}, j:{j}, df[col_left]:{df[col_left]}, df[col_right]:{df[col_right]}")
            pass
    return df


def process_psi(df_filled: pd.DataFrame, categorical_cols: list, psi_select_col: str, psi_select_base: int,
                psi_select_thresh: float, psi_select_bins: int):
    """
    Preprocesses the data by calculating the Population Stability Index (PSI) for a given column.

    Args:
        df_filled (pd.DataFrame): The input DataFrame with missing values filled.
        categorical_cols (list): A list of column names that are categorical variables.
        psi_select_col (str): The name of the column for which PSI is calculated.
        psi_select_base (int): The base period for calculating PSI.
        psi_select_thresh (float): The threshold value for PSI. Columns with PSI above this value will be selected.
        psi_select_bins (int): The number of bins to use for calculating PSI.

    Returns:
        df_filled (pd.DataFrame): The input DataFrame with the PSI values calculated.
    """
    # TODO: 需要验证计算psi的正确性 当为空值时 公式中没有讲如何处理 这里先按照github上的代码处理

    # 最终所有保留的列
    psi_selected_cols = []

    for col_select in df_filled.columns:
        if col_select == psi_select_col:
            # 如果是psi选择的列 则一定不保留
            continue
        elif col_select in categorical_cols:
            # 如果是特征列 则一定保留
            psi_selected_cols.append(col_select)
        else:
            # 如果是数值列 则计算psi 判断是否保留
            # 先提出base和select的数据
            psi_select_col_base_value = df_filled[col_select][df_filled[psi_select_col]
                                                              == psi_select_base].values
            max_psi = 0
            for col_base in set(df_filled[psi_select_col]):
                if col_base == psi_select_base:
                    continue
                else:
                    col_value = df_filled[col_select][df_filled[psi_select_col]
                                                      == col_base].values

                    col_psi = calculate_psi(psi_select_col_base_value, col_value, buckettype='quantiles',
                                            buckets=psi_select_bins, axis=1)
                    # 如果psi值大于阈值 则保留
                    if col_psi > max_psi:
                        max_psi = col_psi
                # 如果小于阈值则保留
            if max_psi < psi_select_thresh:
                psi_selected_cols.append(col_select)

    df_filled = df_filled[psi_selected_cols]
    log.info(f"process_psi psi_selected_cols:{psi_selected_cols}")
    return df_filled, psi_selected_cols


def union_column_info(column_info1: pd.DataFrame, column_info2: pd.DataFrame):
    """
    union the column_info1 with the column_info2.

    Args:
        column_info1 (DataFrame): The column_info1 to be merged.
        column_info2 (DataFrame): The column_info2 to be merged.

    Returns:
        column_info_merge (DataFrame): The union column_info.
    """
    # 将column_info1和column_info2按照left_index=True, right_index=True的方式进行合并 如果列有缺失则赋值为None 行的顺序按照column_info1
    column_info_conbine = column_info1.merge(
        column_info2, how='outer', left_index=True, right_index=True, sort=False)
    col1_index_list = column_info1.index.to_list()
    col2_index_list = column_info2.index.to_list()
    merged_list = col1_index_list + \
        [item for item in col2_index_list if item not in col1_index_list]
    column_info_conbine = column_info_conbine.reindex(merged_list)
    return column_info_conbine
