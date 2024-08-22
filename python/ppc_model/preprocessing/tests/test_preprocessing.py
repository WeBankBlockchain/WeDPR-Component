from ppc_common.ppc_utils import utils
from local_processing.preprocessing import union_column_info, process_na_dataframe, process_na_fill_dataframe, process_outliers, one_hot_encode_and_merge, normalize_dataframe, process_train_dataframe, remove_high_correlation_features, process_psi, process_dataframe
import pandas as pd
import numpy as np
import pytest
import json
import sys

from ppc_model.preprocessing.local_processing.standard_type_enum import standardType
# import pytest
import numpy as np
import pandas as pd
from ppc_model.preprocessing.local_processing.preprocessing import union_column_info, process_na_dataframe, process_na_fill_dataframe, process_outliers, one_hot_encode_and_merge, normalize_dataframe, process_train_dataframe, remove_high_correlation_features, process_psi, process_dataframe
from ppc_common.ppc_utils import utils


def test_process_na_dataframe():
    # Create a sample DataFrame with missing values
    df = pd.DataFrame({
        'col1': [1, 2, None, 4, 5],
        'col2': [None, 2, None, 4, None],
        'col3': [1, 2, 3, 4, 5],
        'col4': [1, None, 3, 4, 5],
        'col5': [1, 2, None, None, None],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [1, 2, 3.0, 4, 5],
        'col3': [1, 2, 3, 4, 5],
        'col4': [1, 6.0, 3, 4, 5],
        'y': [0, 1, 0, 1, 0]
    })
    expected_column_info = {'col1': {'missing_ratio': 0.2, 'na_selected': 1, 'isExisted': True},
                            'col2': {'missing_ratio': 0.6, 'na_selected': 0, 'isExisted': False},
                            'col3': {'missing_ratio': 0.0, 'na_selected': 1, 'isExisted': True},
                            'col4': {'missing_ratio': 0.2, 'na_selected': 1, 'isExisted': True},
                            'col5': {'missing_ratio': 0.6, 'na_selected': 0, 'isExisted': False},
                            'y': {'missing_ratio': 0.0, 'na_selected': 1, 'isExisted': True}}

    # Call the function under test
    processed_df, column_info = process_na_dataframe(df, 0.5)
    assert column_info == expected_column_info
    processed_df = process_na_fill_dataframe(processed_df, ['col2', 'col4'])

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_process_nan_dataframe():
    # Create a sample DataFrame with missing values
    df = pd.DataFrame({
        'col1': [1, 2, None, 4, 5],
        'col2': [None, 2, None, 4, None],
        'col3': [1, 2, 3, 4, 5],
        'col4': [1, None, 3, 4, 5],
        'col5': [1, 2, None, None, None],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [1, 2, np.nan, 4, 5],
        'col2': [np.nan, 2, np.nan, 4, np.nan],
        'col3': [1, 2, 3, 4, 5],
        'col4': [1, np.nan, 3, 4, 5],
        'col5': [1, 2, np.nan, np.nan, np.nan],
        'y': [0, 1, 0, 1, 0]
    })

    # Call the function under test
    processed_df = df.replace('None', np.nan)

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_process_outliers():
    # Create a sample DataFrame with outliers
    df = pd.DataFrame({
        'col3': [100, 200, None, 400, 500, 100, 200, 300, 400, 500, 100, 200, None, 400, 500],
        'col5': [100000, None, 1, -1, 1, -1, 1, -1, 1, 1, -1, None, None, None, 1],
    })
    #
    # outlier = np.random.normal(df['col5'].mean() + 3 * df['col5'].std(), df['col5'].std())
    # df.at[1, 'col5'] = outlier

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col3': [100, 200, None, 400, 500, 100, 200, 300, 400, 500, 100, 200, None, 400, 500],
        'col5': [df['col5'].mean(), None, 1, -1, 1, -1, 1, -1, 1, 1, -1, None, None, None, 1],
    })

    # Call the function under test
    processed_df = process_outliers(df, [])

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_one_hot_encode_and_merge():
    # Create a sample DataFrame
    df = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'y': [0, 1, 0, 1, 0],
        'col2_A': [True, False, False, True, False],
        'col2_B': [False, True, False, False, True],
        'col2_C': [False, False, True, False, False],
        'col3_X': [True, False, False, True, False],
        'col3_Y': [False, True, False, False, True],
        'col3_Z': [False, False, True, False, False]
    })

    # Call the function under test
    processed_df = one_hot_encode_and_merge(df, ['col2', 'col3'])

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_normalize_dataframe():
    # Create a sample DataFrame
    df = pd.DataFrame({
        'col1': [1, 2, None, 4, 5],
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'col4': [10, 20, 30, 40, 50],
        'col5': [100, 200, None, 400, 500]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [0.0, 0.25, None, 0.75, 1.0],
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'col4': [0.0, 0.25, 0.5, 0.75, 1.0],
        'col5': [0.0, 0.25, None, 0.75, 1.0]
    })

    # Call the function under test
    processed_df = normalize_dataframe(
        df, ['col2', 'col3'], standardType.min_max.value)
    assert processed_df.equals(expected_output)

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [-1.265, -0.632, None, 0.632, 1.265],
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'col4': [-1.414, -0.707, 0.0, 0.707, 1.414],
        'col5': [-1.265, -0.632, None, 0.632, 1.265]
    })
    # Call the function under test
    processed_df = normalize_dataframe(
        df, ['col2', 'col3'], standardType.z_score.value)
    # Assert that the processed DataFrame matches the expected output
    print(expected_output)
    print(processed_df)
    assert processed_df.round(3).equals(expected_output)


def test_remove_high_correlation_features():
    # Create a sample DataFrame
    df = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'col2': [2, 4, 6, 8, 10],
        'col3': [3, 6, 9, 12, 15],
        'col4': [4, 8, 12, 16, 20],
        'col5': [5, 10, 15, 20, 25],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'y': [0, 1, 0, 1, 0]
    })

    # Call the function under test
    processed_df = remove_high_correlation_features(df, ['y'], 0.8)

    df = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'col2': [2, 4, None, 8, 10],
        'col3': [3, 6, 9, 12, 15],
        'col4': [4, None, 12, 16, 20],
        'col5': [5, 10, None, 20, 25],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [1, 2, 3, 4, 5],
        'y': [0, 1, 0, 1, 0]
    })
    processed_df = remove_high_correlation_features(df, ['y'], 0.8)
    print(f"processed_df:{processed_df}")
    print(f"expected_output:{expected_output}")
    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)

    df = pd.DataFrame({
        'col1': [-1, 222, 3.4, -22, 5.1],
        'col2': [2, None, 6, 8, 10],
        'col3': [3, 6, 9, 12, 15],
        'col4': [4, None, 12, 16, 20],
        'col5': [5, 10, None, 20, 25],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col1': [-1, 222, 3.4, -22, 5.1],
        'col2': [2, None, 6, 8, 10],
        'y': [0, 1, 0, 1, 0]
    })
    processed_df = remove_high_correlation_features(df, ['y'], 0.8)

    print(processed_df)

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_process_psi():
    # Create a sample DataFrame
    df_filled = pd.DataFrame({
        # 'col1': [0, 1, 0, 1, 0],
        'col1': [1, 2, 3, 4, 5],
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'col4': [10, 20, 30, 40, 50],
        'col5': [100, 200, 300, 400, 500],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'col2': ['A', 'B', 'C', 'A', 'B'],
        'col3': ['X', 'Y', 'Z', 'X', 'Y'],
        'y': [0, 1, 0, 1, 0]
    })

    # Call the function under test
    processed_df, _ = process_psi(
        df_filled, ['col2', 'col3', 'y'], 'col1', 1, 0.9, 5)

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)

    # read test csv as DataFrame
    test_file_path = "./癌症纵向训练数据集psi.csv"
    df_filled = pd.read_csv(test_file_path)
    expected_output_col = ['id', 'y', 'x1', 'x5',
                           'x6', 'x9', 'x10', 'x11', 'x12', 'x14']
    processed_df, _ = process_psi(
        df_filled, ['id', 'y', 'x1', 'x5', 'x23'], 'x15', 0, 0.3, 4)

    assert processed_df.columns.tolist() == expected_output_col


def test_process_dataframe():
    # Create a sample DataFrame
    test_file_path = "./癌症纵向训练数据集psi.csv"
    df_filled = pd.read_csv(test_file_path)

    # Create a mock JobContext object
    xgb_model = {
        "algorithm_subtype": "HeteroXGB",
        "participants": 2,  # 提供数据集机构数
        # XGBoost参数
        "use_psi": 0,  # 0否，1是
        "use_goss": 0,  # 0否，1是
        "test_dataset_percentage": 0.3,
        "learning_rate": 0.1,
        "num_trees": 6,
        "max_depth": 3,
        "max_bin": 4,  # 分箱数（计算XGB直方图）
        "threads": 8,
        # 调度服务各方预处理
        "na_select": 0.8,  # 缺失值筛选阈值，缺失值比例超过阈值则移除该特征（0表示只要有缺失值就移除，1表示仅移除全为缺失值的列）
        "fillna": 0,  # 是否缺失值填充（均值）
        "psi_select_col": "x15",  # PSI稳定性筛选时间列名称（0代表不进行PSI筛选，例如："month"）
        "psi_select_base": 1,  # PSI稳定性筛选时间基期（例如以0为基期，统计其他时间的psi，时间按照周/月份/季度等提前处理为0,1,2,...,n格式）
        "psi_select_thresh": 0.3,  # 若最大的逐月PSI>0.1，则剔除该特征
        "psi_select_bins": 4,  # 计算逐月PSI时分箱数
        "filloutlier": 1,  # 是否异常值处理（+-3倍标准差使用均值填充）
        "normalized": 1,  # 是否归一化，每个值减去最小值，然后除以最大值与最小值的差
        "standardized": 1,  # 是否标准化，计算每个数据点与均值的差，然后除以标准差
        "one_hot": 0,  # 是否进行onehot
        # 指定分类特征索引，例如"x1,x12,x23" ("0"代表无分类特征)（分类特征需要在此处标注，使用onehot预处理/建模）
        "categorical": "id,y,x1,x5,x23",
        # 建模节点特征工程
        "use_iv": 0,  # 是否计算woe/iv，并使用iv进行特征筛选
        'group_num': 3,  # 分箱数（计算woe分箱，等频）
        'iv_thresh': 0.1,  # 使用iv进行特征筛选的阈值（仅保留iv大于阈值的特征）
        'corr_select': 0.8  # 计算特征相关性，相关性大于阈值特征仅保留iv最大的（如use_iv=0，则随机保留一个）（corr_select=0时不进行相关性筛选）
    }

    xgb_dict = dict()
    # 根据xgb_model 生成xgb_dict
    for key, value in xgb_model.items():
        xgb_dict[key] = value

    # Call the function under test
    column_info1 = process_dataframe(
        df_filled, xgb_dict, "./xgb_data_file_path", utils.AlgorithmType.Train.name, "j-123456")

    xgb_model = {
        "algorithm_subtype": "HeteroXGB",
        "participants": 2,  # 提供数据集机构数
        # XGBoost参数
        "use_psi": 0,  # 0否，1是
        "use_goss": 0,  # 0否，1是
        "test_dataset_percentage": 0.3,
        "learning_rate": 0.1,
        "num_trees": 6,
        "max_depth": 3,
        "max_bin": 4,  # 分箱数（计算XGB直方图）
        "threads": 8,
        # 调度服务各方预处理
        "na_select": 0.8,  # 缺失值筛选阈值，缺失值比例超过阈值则移除该特征（0表示只要有缺失值就移除，1表示仅移除全为缺失值的列）
        "fillna": 1,  # 是否缺失值填充（均值）
        "psi_select_col": "x15",  # PSI稳定性筛选时间列名称（0代表不进行PSI筛选，例如："month"）
        "psi_select_base": 1,  # PSI稳定性筛选时间基期（例如以0为基期，统计其他时间的psi，时间按照周/月份/季度等提前处理为0,1,2,...,n格式）
        "psi_select_thresh": 0.3,  # 若最大的逐月PSI>0.1，则剔除该特征
        "psi_select_bins": 4,  # 计算逐月PSI时分箱数
        "filloutlier": 1,  # 是否异常值处理（+-3倍标准差使用均值填充）
        "normalized": 1,  # 是否归一化，每个值减去最小值，然后除以最大值与最小值的差
        "standardized": 1,  # 是否标准化，计算每个数据点与均值的差，然后除以标准差
        "one_hot": 0,  # 是否进行onehot
        # 指定分类特征索引，例如"x1,x12,x23" ("0"代表无分类特征)（分类特征需要在此处标注，使用onehot预处理/建模）
        "categorical": "id,y,x1,x5,x23",
        # 建模节点特征工程
        "use_iv": 0,  # 是否计算woe/iv，并使用iv进行特征筛选
        'group_num': 3,  # 分箱数（计算woe分箱，等频）
        'iv_thresh': 0.1,  # 使用iv进行特征筛选的阈值（仅保留iv大于阈值的特征）
        'corr_select': 0.8  # 计算特征相关性，相关性大于阈值特征仅保留iv最大的（如use_iv=0，则随机保留一个）（corr_select=0时不进行相关性筛选）
    }
    for key, value in xgb_model.items():
        xgb_dict[key] = value
    column_info2 = process_dataframe(
        df_filled, xgb_dict, "./xgb_data_file_path2", utils.AlgorithmType.Train.name, "j-123456")
    assert column_info1 == column_info2
    expected_column_info = {'y': {'missing_ratio': 0.0,
                                  'na_selected': 1,
                                  'psi_selected': 1,
                                  'corr_selected': 1,
                                  'isExisted': True
                                  },
                            'x0': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x1': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x2': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 0,
                                   'isExisted': False
                                   },
                            'x3': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 0,
                                   'isExisted': False
                                   },
                            'x4': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 0,
                                   'corr_selected': 0,
                                   'isExisted': False
                                   },
                            'x5': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x6': {'missing_ratio': 0.018,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x7': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 0,
                                   'corr_selected': 0,
                                   'isExisted': False
                                   },
                            'x8': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x9': {'missing_ratio': 0.0,
                                   'na_selected': 1,
                                   'psi_selected': 1,
                                   'corr_selected': 1,
                                   'isExisted': True
                                   },
                            'x10': {'missing_ratio': 0.012,
                                    'na_selected': 1,
                                    'psi_selected': 1,
                                    'corr_selected': 1,
                                    'isExisted': True
                                    },
                            'x11': {'missing_ratio': 0.998,
                                    'na_selected': 0,
                                    'psi_selected': 0,
                                    'corr_selected': 0,
                                    'isExisted': False
                                    },
                            'x12': {'missing_ratio': 0.0,
                                    'na_selected': 1,
                                    'psi_selected': 1,
                                    'corr_selected': 0,
                                    'isExisted': False
                                    },
                            'x13': {'missing_ratio': 0.0,
                                    'na_selected': 1,
                                    'psi_selected': 1,
                                    'corr_selected': 0,
                                    'isExisted': False
                                    },
                            'x14': {'missing_ratio': 0.0,
                                    'na_selected': 1,
                                    'psi_selected': 1,
                                    'corr_selected': 1,
                                    'isExisted': True
                                    },
                            'x15': {'missing_ratio': 0.0,
                                    'na_selected': 1,
                                    'psi_selected': 0,
                                    'corr_selected': 0,
                                    'isExisted': False
                                    }
                            }
    # 转成python字典
    expected_column_info_dict = dict(expected_column_info)
    assert column_info1 == expected_column_info_dict


def test_process_train_dataframe():
    # Create a sample DataFrame
    df = pd.DataFrame({
        'id': [1, 2, 3, 4, 5],
        'x1': [10, 20, 30, 40, 50],
        'x2': [100, 200, 300, 400, 500],
        'x3': [1000, 2000, 3000, 4000, 5000],
        'x4': [10000, 20000, 30000, 40000, 50000],
        'x5': [100000, 200000, 300000, 400000, 500000],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'id': [1, 2, 3, 4, 5],
        'x2': [100, 200, 300, 400, 500],
        'x3': [1000, 2000, 3000, 4000, 5000]
    })

    # Define the column_info dictionary
    column_info = {
        'id': {'isExisted': True},
        'x1': {'isExisted': False},
        'x2': {'isExisted': True},
        'x3': {'isExisted': True},
        'x4': {'isExisted': False},
        'x5': {'isExisted': False},
        'y': {'isExisted': False},
    }

    # Call the function under test
    processed_df = process_train_dataframe(df, column_info)

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_process_train_dataframe_with_additional_columns():
    # Create a sample DataFrame
    df = pd.DataFrame({
        'id': [1, 2, 3, 4, 5],
        'x1': [10, 20, 30, 40, 50],
        'x2': [100, 200, 300, 400, 500],
        'x3': [1000, 2000, 3000, 4000, 5000],
        'x4': [10000, 20000, 30000, 40000, 50000],
        'x5': [100000, 200000, 300000, 400000, 500000],
        'y': [0, 1, 0, 1, 0]
    })

    # Define the expected output DataFrame
    expected_output = pd.DataFrame({
        'id': [1, 2, 3, 4, 5],
        'x1': [10, 20, 30, 40, 50],
        'x3': [1000, 2000, 3000, 4000, 5000],
        'x4': [10000, 20000, 30000, 40000, 50000],
        'x5': [100000, 200000, 300000, 400000, 500000]
    })

    # Define the column_info dictionary
    column_info = {
        'id': {'isExisted': True},
        'x1': {'isExisted': True},
        'x2': {'isExisted': False},
        'x3': {'isExisted': True},
        'x4': {'isExisted': True},
        'x5': {'isExisted': True},
        'y': {'isExisted': False},
    }

    # Call the function under test
    processed_df = process_train_dataframe(df, column_info)

    # Assert that the processed DataFrame matches the expected output
    assert processed_df.equals(expected_output)


def test_merge_column_info_from_file():
    col_info_file_path = "./test_column_info_merge.csv"
    iv_info_file_path = "./test_column_info_iv.csv"
    column_info_fm = pd.read_csv(col_info_file_path, index_col=0)
    column_info_iv = pd.read_csv(iv_info_file_path, index_col=0)
    union_df = union_column_info(column_info_fm, column_info_iv)

    col_str_expected = '{"y": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x0": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x1": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x2": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 1, "corr_selected": 0}, "x3": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 1, "corr_selected": 0}, "x4": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 0, "corr_selected": 0}, "x5": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x6": {"missing_ratio": 0.018, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x7": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 0, "corr_selected": 0}, "x8": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x9": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x10": {"missing_ratio": 0.012, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x11": {"missing_ratio": 0.998, "na_selected": 0, "isExisted": false, "psi_selected": 0, "corr_selected": 0}, "x12": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 1, "corr_selected": 0}, "x13": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 1, "corr_selected": 0}, "x14": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": true, "psi_selected": 1, "corr_selected": 1}, "x15": {"missing_ratio": 0.0, "na_selected": 1, "isExisted": false, "psi_selected": 0, "corr_selected": 0}}'

    # expected_df_file_path = './test_union_column.csv'
    # expected_df = pd.read_csv(expected_df_file_path, index_col=0)
    # assert expected_df.equals(union_df)
    column_info_str = json.dumps(column_info_fm.to_dict(orient='index'))
    assert column_info_str == col_str_expected


def construct_dataset(num_samples, num_features, file_path):
    np.random.seed(0)
    # 生成标签列
    labels = np.random.choice([0, 1], size=num_samples)
    # 生成特征列
    features = np.random.rand(num_samples, num_features)
    # 将标签转换为DataFrame
    labels_df = pd.DataFrame(labels, columns=['Label'])

    # 将特征转换为DataFrame
    features_df = pd.DataFrame(features)

    # 合并标签和特征DataFrame
    dataset_df = pd.concat([labels_df, features_df], axis=1)

    # 将DataFrame写入CSV文件
    dataset_df.to_csv(file_path, index=False)

    return labels, features


def test_gen_file():
    num_samples = 400000
    num_features = 100
    file_path = "./dataset-{}-{}.csv".format(num_samples, num_features)
    construct_dataset(num_samples, num_features, file_path)


def test_large_process_train_dataframe():
    num_samples = 400000
    num_features = 100
    test_file_path = "./dataset-{}-{}.csv".format(num_samples, num_features)
    df_filled = pd.read_csv(test_file_path)

    # Create a mock JobContext object
    xgb_model = {
        "algorithm_subtype": "HeteroXGB",
        "participants": 2,  # 提供数据集机构数
        # XGBoost参数
        "use_psi": 0,  # 0否，1是
        "use_goss": 0,  # 0否，1是
        "test_dataset_percentage": 0.3,
        "learning_rate": 0.1,
        "num_trees": 6,
        "max_depth": 3,
        "max_bin": 4,  # 分箱数（计算XGB直方图）
        "threads": 8,
        # 调度服务各方预处理
        "na_select": 0.8,  # 缺失值筛选阈值，缺失值比例超过阈值则移除该特征（0表示只要有缺失值就移除，1表示仅移除全为缺失值的列）
        "fillna": 0,  # 是否缺失值填充（均值）
        "psi_select_col": 0,  # PSI稳定性筛选时间列名称（0代表不进行PSI筛选，例如："month"）
        "psi_select_base": 1,  # PSI稳定性筛选时间基期（例如以0为基期，统计其他时间的psi，时间按照周/月份/季度等提前处理为0,1,2,...,n格式）
        "psi_select_thresh": 0.3,  # 若最大的逐月PSI>0.1，则剔除该特征
        "psi_select_bins": 4,  # 计算逐月PSI时分箱数
        "filloutlier": 1,  # 是否异常值处理（+-3倍标准差使用均值填充）
        "normalized": 1,  # 是否归一化，每个值减去最小值，然后除以最大值与最小值的差
        "standardized": 1,  # 是否标准化，计算每个数据点与均值的差，然后除以标准差
        "one_hot": 0,  # 是否进行onehot
        # 指定分类特征索引，例如"x1,x12,x23" ("0"代表无分类特征)（分类特征需要在此处标注，使用onehot预处理/建模）
        "categorical": "id,y,1,5,23",
        # 建模节点特征工程
        "use_iv": 0,  # 是否计算woe/iv，并使用iv进行特征筛选
        'group_num': 3,  # 分箱数（计算woe分箱，等频）
        'iv_thresh': 0.1,  # 使用iv进行特征筛选的阈值（仅保留iv大于阈值的特征）
        'corr_select': 0.8  # 计算特征相关性，相关性大于阈值特征仅保留iv最大的（如use_iv=0，则随机保留一个）（corr_select=0时不进行相关性筛选）
    }

    xgb_dict = dict()
    # 根据xgb_model 生成xgb_dict
    for key, value in xgb_model.items():
        xgb_dict[key] = value

    # Call the function under test
    start_time = time.time()
    column_info1 = process_dataframe(
        df_filled, xgb_dict, "./xgb_data_file_path", utils.AlgorithmType.Train.name, "j-123456")
    end_time = time.time()
    print(
        f"test_large_process_train_dataframe time cost:{end_time-start_time}, num_samples: {num_samples}, num_features: {num_features}")


# Run the tests
# pytest.main()
if __name__ == "__main__":
    import time
    # test_large_process_train_dataframe()
    time1 = time.time()
    test_process_na_dataframe()
    time2 = time.time()
    test_process_nan_dataframe()
    time3 = time.time()
    test_process_outliers()
    time4 = time.time()
    test_one_hot_encode_and_merge()
    time5 = time.time()
    test_normalize_dataframe()
    time6 = time.time()
    test_remove_high_correlation_features()
    time7 = time.time()
    test_process_psi()
    time8 = time.time()
    test_process_dataframe()
    time9 = time.time()
    test_process_train_dataframe()
    time10 = time.time()
    test_process_train_dataframe_with_additional_columns()
    time11 = time.time()
    test_merge_column_info_from_file()
    time12 = time.time()
    print(f"test_process_na_dataframe time cost: {time2-time1}")
    print(f"test_process_nan_dataframe time cost: {time3-time2}")
    print(f"test_process_outliers time cost: {time4-time3}")
    print(f"test_one_hot_encode_and_merge time cost: {time5-time4}")
    print(f"test_normalize_dataframe time cosy: {time6-time5}")
    print(f"test_remove_high_correlation_features time cost: {time7-time6}")
    print(f"test_process_psi time cost: {time8-time7}")
    print(f"test_process_dataframe time cost: {time9-time8}")
    print(f"test_process_train_dataframe time cost: {time10-time9}")
    print(
        f"test_process_train_dataframe_with_additional_columns time cost: {time11-time10}")
    print(f"test_merge_column_info_from_file time cost: {time12-time11}")
    print("All tests pass!")
