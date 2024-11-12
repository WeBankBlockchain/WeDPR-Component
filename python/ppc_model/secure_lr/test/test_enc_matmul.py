import unittest
import numpy as np

from ppc_model.common.initializer import Initializer
from ppc_model.secure_lr.secure_lr_context import SecureLRContext
from ppc_model.secure_lr.vertical.booster import VerticalBooster


ACTIVE_PARTY = 'ACTIVE_PARTY'

job_id = 'j-1234'
task_id = 't-1234'

model_dict = {
    'objective': 'regression',
    'categorical_feature': [],
    'train_features': "",
    'epochs': 1,
    'batch_size': 8,
    'feature_rate': 0.8,
    'random_state': 2024
}

args = {
    'job_id': job_id,
    'task_id': task_id,
    'is_label_holder': True,
    'result_receiver_id_list': [],
    'participant_id_list': [],
    'model_predict_algorithm': None,
    'algorithm_type': 'Train',
    'algorithm_subtype': 'HeteroXGB',
    'model_dict': model_dict
}


class TestEncMatmul(unittest.TestCase):

    def test_enc_matmul(self):
        active_components = Initializer(log_config_path='', config_path='')
        active_components.config_data = {
            'JOB_TEMP_DIR': '/tmp/active', 'AGENCY_ID': ACTIVE_PARTY}
        task_info = SecureLRContext(args['task_id'], args, active_components)
        
        # 15个特征，batch_size: 8
        arr = np.array([2, 4, -5, 0, 9, -7, 12, 3])
        np.random.seed(0)
        # x = np.random.randint(-10, 10, size=(15, 8))
        x = np.random.randint(0, 10, size=(15, 8))
        enc_arr = task_info.phe.encrypt_batch_parallel((arr).astype('object'))
        enc_x_d = VerticalBooster.enc_matmul(x, enc_arr)
        x_d_rec = np.array(task_info.phe.decrypt_batch(enc_x_d), dtype='object')
        x_d_rec[x_d_rec > 2**(task_info.phe.key_length-1)] -= 2**(task_info.phe.key_length)

        assert (np.matmul(x, arr) == x_d_rec).all()

        arr_ = VerticalBooster.rounding_d(arr)
        x_ = VerticalBooster.rounding_d(x)
        enc_arr = task_info.phe.encrypt_batch_parallel((arr_).astype('object'))
        enc_x_d = VerticalBooster.enc_matmul(x_, enc_arr)
        x_d_rec = np.array(task_info.phe.decrypt_batch(enc_x_d), dtype='object')
        x_d_rec = VerticalBooster.recover_d(task_info, x_d_rec, is_square=True)

        assert (np.matmul(x, arr) == x_d_rec).all()
