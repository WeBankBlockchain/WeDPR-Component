import os
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split

from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_common.ppc_utils.utils import AlgorithmType
from ppc_model.common.protocol import TaskRole
from ppc_model.common.model_result import ResultFileHandling, CommonMessage, SendMessage
from ppc_model.secure_lgbm.secure_lgbm_context import SecureLGBMContext


class SecureDataset:
    def __init__(self, ctx: SecureLGBMContext, model_data=None, delimiter: str = ' '):

        self.eval_column_file = ctx.eval_column_file
        self.iv_selected_file = ctx.iv_selected_file
        self.selected_col_file = ctx.selected_col_file
        self.is_label_holder = ctx.is_label_holder
        self.algorithm_type = ctx.algorithm_type
        self.test_size = ctx.model_params.test_size
        self.random_state = ctx.model_params.random_state
        self.eval_set_column = ctx.model_params.eval_set_column
        self.train_set_value = ctx.model_params.train_set_value
        self.eval_set_value = ctx.model_params.eval_set_value

        self.ctx = ctx
        self.train_X = None
        self.test_X = None
        self.train_y = None
        self.test_y = None
        self.train_idx = None
        self.test_idx = None
        self.feature_name = None

        if model_data is None:
            self.model_data = pd.read_csv(
                ctx.model_prepare_file, header=0, delimiter=delimiter)
        else:
            self.model_data = model_data

        self._construct_dataset()

    @staticmethod
    def read_dataset(file_path, has_label: bool, delimiter: str = ' '):
        df = pd.read_csv(file_path, header=0, delimiter=delimiter)

        if 'id' in df.columns:
            df.drop('id', axis=1, inplace=True)

        field_list = df.columns.tolist()

        if has_label:
            return field_list, df.iloc[:, 0].values, df.iloc[:, 1:].values
        else:
            return field_list, None, df.iloc[:, :].values

    @staticmethod
    def simulate_dataset(data_size: int = 1000, feature_dim: int = 100, has_label: bool = True):
        X = np.random.rand(data_size, feature_dim)
        if has_label:
            y = np.random.randint(2, size=data_size)
            return SecureDataset.assembling_dataset(X, y)
        return SecureDataset.assembling_dataset(X)

    @staticmethod
    def assembling_dataset(X: np.ndarray, y: np.ndarray = None):
        # 创建自定义列名
        num_features = X.shape[1]
        column_names = [f'x{i+1}' for i in range(num_features)]

        # 创建数据框并使用自定义列名
        df = pd.DataFrame(X, columns=column_names)

        # 添加 id 列和 y 列
        df.insert(0, 'id', range(1, len(df) + 1))
        if y is not None:
            df.insert(1, 'y', y)

        return df

    @staticmethod
    def hetero_split_dataset(df: pd.DataFrame, split_point: int = None):
        # 获取特征列名
        column_names = df.columns.tolist()
        if 'id' in column_names:
            column_names.remove('id')
        if 'y' in column_names:
            column_names.remove('y')

        # 计算切分点
        if not split_point:
            split_point = (df.shape[1] - 2) // 2

        # 划分特征列
        columns_with_y = ['id', 'y'] + column_names[:split_point]
        columns_without_y = ['id'] + column_names[split_point:]

        # 创建两个数据集
        df_with_y = df[columns_with_y]
        df_without_y = df[columns_without_y]

        return df_with_y, df_without_y

    def _random_split_dataset(self):

        train_data, test_data = train_test_split(
            self.model_data, test_size=self.test_size, random_state=self.random_state)

        return train_data, test_data

    def _customized_split_dataset(self):
        if self.ctx.role == TaskRole.ACTIVE_PARTY:
            for partner_index in range(1, len(self.ctx.participant_id_list)):
                byte_data = SendMessage._receive_byte_data(self.ctx.components.stub, self.ctx,
                                                           f'{CommonMessage.EVAL_SET_FILE.value}', partner_index)
                if not os.path.exists(self.eval_column_file) and byte_data != bytes():
                    with open(self.eval_column_file, 'wb') as f:
                        f.write(byte_data)
            with open(self.eval_column_file, 'rb') as f:
                byte_data = f.read()
            for partner_index in range(1, len(self.ctx.participant_id_list)):
                SendMessage._send_byte_data(self.ctx.components.stub, self.ctx, f'{CommonMessage.EVAL_SET_FILE.value}',
                                            byte_data, partner_index)
        else:
            if not os.path.exists(self.eval_column_file):
                byte_data = bytes()
            else:
                with open(self.eval_column_file, 'rb') as f:
                    byte_data = f.read()
            SendMessage._send_byte_data(self.ctx.components.stub, self.ctx, f'{CommonMessage.EVAL_SET_FILE.value}',
                                        byte_data, 0)
            byte_data = SendMessage._receive_byte_data(self.ctx.components.stub, self.ctx,
                                                       f'{CommonMessage.EVAL_SET_FILE.value}', 0)
            if not os.path.exists(self.eval_column_file):
                with open(self.eval_column_file, 'wb') as f:
                    f.write(byte_data)

        eval_set_df = pd.read_csv(self.eval_column_file, header=0)
        train_data = self.model_data[eval_set_df[self.eval_set_column]
                                     == self.train_set_value]
        test_data = self.model_data[eval_set_df[self.eval_set_column]
                                    == self.eval_set_value]

        return train_data, test_data

    def _construct_model_dataset(self, train_data, test_data):

        self.train_idx = train_data['id'].values
        self.test_idx = test_data['id'].values

        if self.is_label_holder and 'y' in train_data.columns:
            self.train_y = train_data['y'].values
            self.test_y = test_data['y'].values
            self.train_X = train_data.drop(columns=['id', 'y']).values
            self.test_X = test_data.drop(columns=['id', 'y']).values
            self.feature_name = train_data.drop(
                columns=['id', 'y']).columns.tolist()
        else:
            self.train_X = train_data.drop(columns=['id']).values
            self.test_X = test_data.drop(columns=['id']).values
            self.feature_name = train_data.drop(
                columns=['id']).columns.tolist()

    def _construct_predict_dataset(self, test_data):
        self.test_idx = test_data['id'].values
        if self.is_label_holder and 'y' in test_data.columns:
            self.test_y = test_data['y'].values
            self.test_X = test_data.drop(columns=['id', 'y']).values
            self.feature_name = test_data.drop(
                columns=['id', 'y']).columns.tolist()
        else:
            self.test_X = test_data.drop(columns=['id']).values
            self.feature_name = test_data.drop(columns=['id']).columns.tolist()

    def _dataset_fe_selected(self, file_path, feature_name):
        iv_selected = pd.read_csv(file_path, header=0)
        selected_list = iv_selected[feature_name][iv_selected['iv_selected'] == 1].tolist(
        )

        drop_columns = []
        for column in self.model_data.columns:
            if column == 'id' or column == 'y':
                continue
            if column not in selected_list:
                drop_columns.append(column)

        if len(drop_columns) > 0:
            self.model_data = self.model_data.drop(columns=drop_columns)

    def _construct_dataset(self):
        if self.algorithm_type == AlgorithmType.Predict.name:
            my_fields = []
            for item in self.ctx.model_predict_algorithm['participant_agency_list']:
                if item["agency"] == self.ctx.components.config_data['AGENCY_ID']:
                    my_fields = item["fields"]
            if 'y' in self.model_data.columns and 'y' not in my_fields:
                my_fields = ['y'] + my_fields
            if 'id' in self.model_data.columns and 'id' not in my_fields:
                my_fields = ['id'] + my_fields
            self.model_data = self.model_data[my_fields]

        if os.path.exists(self.iv_selected_file):
            self._dataset_fe_selected(self.iv_selected_file, 'feature')

        if self.algorithm_type == AlgorithmType.Predict.name \
                and not os.path.exists(self.selected_col_file):
            try:
                self.ctx.remote_selected_col_file = os.path.join(
                    self.ctx.model_params.training_job_id, self.ctx.SELECTED_COL_FILE)
                ResultFileHandling._download_file(self.ctx.components.storage_client,
                                                  self.selected_col_file, self.ctx.remote_selected_col_file)
                self._dataset_fe_selected(self.selected_col_file, 'id')
            except:
                pass

        if 'id' not in self.model_data.columns:
            if self.ctx.dataset_file_path is None:
                import glob
                pattern = os.path.join(self.ctx.workspace, 'd-*')
                dataset_file_path = glob.glob(pattern)[0]
            else:
                dataset_file_path = self.ctx.dataset_file_path
            dataset_id = pd.read_csv(
                dataset_file_path, header=0, usecols=['id'])
            if os.path.exists(self.ctx.psi_result_path):
                psi_data = pd.read_csv(self.ctx.psi_result_path, header=0)
                dataset_id = pd.merge(dataset_id, psi_data, on=[
                                      'id']).sort_values(by='id', ascending=True)
            self.model_data = pd.concat([dataset_id, self.model_data], axis=1)

        if self.algorithm_type == AlgorithmType.Train.name:
            if self.eval_set_column:
                train_data, test_data = self._customized_split_dataset()
            else:
                train_data, test_data = self._random_split_dataset()
            self._construct_model_dataset(train_data, test_data)

        elif self.algorithm_type == AlgorithmType.Predict.name:
            test_data = self.model_data
            self._construct_predict_dataset(test_data)
        else:
            raise PpcException(PpcErrorCode.ALGORITHM_TYPE_ERROR.get_code(),
                               PpcErrorCode.ALGORITHM_TYPE_ERROR.get_message())
