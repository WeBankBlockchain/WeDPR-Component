import hashlib
import json
import logging
import os
import random
import string
import unittest
import uuid
from io import BytesIO

import pandas as pd

from ppc_common.ppc_utils import utils, http_utils
from ppc_common.ppc_crypto import crypto_utils
from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode

log = logging.getLogger(__name__)


def make_hash(data):
    m = hashlib.sha3_256()
    m.update(data)
    return m.hexdigest()


def requester_gen_ot_cipher(id_list, obfuscation_order):
    blinding_a = crypto_utils.get_random_int()
    blinding_b = crypto_utils.get_random_int()
    x_value = crypto_utils.ot_base_pown(blinding_a)
    y_value = crypto_utils.ot_base_pown(blinding_b)
    c_blinding = crypto_utils.ot_mul_fi(blinding_a, blinding_b)
    id_index_list = []
    send_hash_vec = []
    z_value_list = []
    for id_hash in id_list:
        obs_list = id_obfuscation(obfuscation_order, None)
        id_index = random.randint(0, obfuscation_order)
        obs_list[id_index] = id_hash
        id_index_list.append(id_index)
        send_hash_vec.append(obs_list)
        z_value = crypto_utils.ot_base_pown(c_blinding - id_index)
        z_value_list.append(str(z_value))
    return id_index_list, blinding_b, x_value, y_value, send_hash_vec, z_value_list


def provider_gen_ot_cipher(x_value, y_value, send_hash_vec, z_value_list, data_map, is_contain_result=True):
    """
    data_map = hashmap[id1: "message1",
                id2: "message2"]
    """
    if isinstance(x_value, str):
        x_value = int(x_value)
    if isinstance(y_value, str):
        y_value = int(y_value)
    if len(send_hash_vec) != len(z_value_list):
        raise PpcException(PpcErrorCode.AYS_LENGTH_ERROR.get_code(),
                           PpcErrorCode.AYS_LENGTH_ERROR.get_msg())
    message_cipher_vec = []
    for idx, z_value in enumerate(z_value_list):
        if isinstance(z_value, str):
            z_value = int(z_value)
        message_cipher_list = []
        message_int_len = 0
        for send_hash in send_hash_vec[idx]:
            blinding_r = crypto_utils.get_random_int()
            blinding_s = crypto_utils.get_random_int()
            w_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(x_value, blinding_s),
                                            crypto_utils.ot_base_pown(blinding_r))
            key_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(z_value, blinding_s),
                                              crypto_utils.ot_pown(y_value, blinding_r))
            aes_key_bytes = os.urandom(16)
            aes_default = utils.AESCipher(aes_key_bytes)
            aes_key_base64str = utils.bytes_to_base64str(aes_key_bytes)
            message_int, message_int_len = crypto_utils.ot_str_to_int(
                aes_key_base64str)

            if send_hash not in data_map:
                letters = string.ascii_lowercase
                # random string
                ot_message_str = 'message not found'
                cipher = aes_default.encrypt(ot_message_str)
                cipher_str = utils.bytes_to_base64str(cipher)
            else:
                if is_contain_result:
                    ot_message_str = json.dumps(data_map[send_hash])
                    cipher = aes_default.encrypt(ot_message_str)
                    cipher_str = utils.bytes_to_base64str(cipher)
                else:
                    ot_message_str = 'True'
                    cipher = aes_default.encrypt(ot_message_str)
                    cipher_str = utils.bytes_to_base64str(cipher)
            message_cipher = key_value ^ message_int
            z_value = crypto_utils.ot_mul_n(z_value, crypto_utils.DEFAULT_G)
            message_cipher_list.append({
                "w": str(w_value),
                "e": str(message_cipher),
                "len": message_int_len,
                'aesCipher': cipher_str,
            })
        message_cipher_vec.append(message_cipher_list)
    return message_cipher_vec


def requester_ot_recover_result(id_index_list, blinding_b, message_cipher_vec):
    if len(id_index_list) != len(message_cipher_vec):
        raise PpcException(PpcErrorCode.AYS_LENGTH_ERROR.get_code(),
                           PpcErrorCode.AYS_LENGTH_ERROR.get_msg())
    result_list = []
    for idx, id_index in enumerate(id_index_list):
        w_value = message_cipher_vec[idx][id_index]['w']
        e_value = message_cipher_vec[idx][id_index]['e']
        message_len = message_cipher_vec[idx][id_index]['len']
        cipher_str = message_cipher_vec[idx][id_index]['aesCipher']

        if isinstance(w_value, str):
            w_value = int(w_value)
        if isinstance(e_value, str):
            e_value = int(e_value)
        w_1 = crypto_utils.ot_pown(w_value, blinding_b)
        message_recover = w_1 ^ e_value
        try:
            aes_key = crypto_utils.ot_int_to_str(message_recover, message_len)
            key_recover = utils.base64str_to_bytes(aes_key)
            aes_recover = utils.AESCipher(key_recover)
            cipher_recover = utils.base64str_to_bytes(cipher_str)
            message_result = aes_recover.decrypt(cipher_recover)
            result_list.append(message_result)
        except Exception as be:
            result_list.append(None)
    return result_list


def id_obfuscation(obfuscation_order, rule=None):
    # use rule extend different id order, such as driver card or name:
    if rule is not None:
        print('obfuscation is work in progress')
    obs_list = []
    for i in range(0, obfuscation_order+1):
        obs_list.append(make_hash(bytes(str(uuid.uuid4()), 'utf8')))
    return obs_list


def prepare_dataset_with_matrix(search_id_matrix, tmp_file_path, prepare_dataset_tmp_file_path):
    search_reg = 'ppc-normal-prefix'
    for search_list in search_id_matrix:
        for search_id in search_list:
            search_reg = '{}|{}'.format(search_reg, search_id)
    exec_command = 'head -n 1 {} >> {}'.format(
        tmp_file_path, prepare_dataset_tmp_file_path)
    (status, result) = utils.getstatusoutput(exec_command)
    if status != 0:
        log.error(
            f'[OnError]prepare_dataset_with_matrix! status is {status}, output is {result}')
    else:
        log.info(
            f'prepare_dataset_with_matrix success! status is {status}, output is {result}')
    exec_command = 'grep -E \'{}\' {} >> {}'.format(
        search_reg, tmp_file_path, prepare_dataset_tmp_file_path)
    (status, result) = utils.getstatusoutput(exec_command)
    if status != 0:
        log.error(
            f'[OnError]prepare_dataset_with_matrix! status is {status}, output is {result}')
    else:
        log.info(
            f'prepare_dataset_with_matrix success! status is {status}, output is {result}')


class TestOtMethods(unittest.TestCase):

    def test_choice_all_flow(self):
        file_path = '/Users/asher/Downloads/test_file.csv'
        # data_pd = pd.read_csv(file_path, index_col=0, header=0)
        data_pd = pd.read_csv(file_path)
        data_map = data_pd.set_index(data_pd.columns[0]).T.to_dict('list')

        choice = ['bob', '小鸡', '美丽']
        obs_order = 10
        _id_index_list, _blinding_b, _x_value, _y_value, _send_hash_vec, _z_value_list = requester_gen_ot_cipher(
            choice, obs_order)
        _message_cipher_vec = provider_gen_ot_cipher(
            _x_value, _y_value, _send_hash_vec, _z_value_list, data_map)
        result = requester_ot_recover_result(
            _id_index_list, _blinding_b, _message_cipher_vec)
        for idx, id_num in enumerate(choice):
            print(f"found {id_num} value is {result[idx]}")

    # def test_choice_ot(self):
    #     choice_list = [1, 4]
    #     blinding_a = crypto_utils.get_random_int()
    #     blinding_b = crypto_utils.get_random_int()
    #     x_value = crypto_utils.ot_base_pown(blinding_a)
    #     y_value = crypto_utils.ot_base_pown(blinding_b)
    #     c_blinding = crypto_utils.ot_mul_fi(blinding_a, blinding_b)
    #     # c_value = crypto_utils.ot_base_pown(c_blinding)
    #     z_value_list = []
    #     for choice in choice_list:
    #         z_value = crypto_utils.ot_base_pown(c_blinding - choice)
    #         z_value_list.append(z_value)
    #     # send x_value, y_value, z_value
    #     message_str_list = ['hello', 'world', 'ot', 'cipher', 'test']
    #     message_list = []
    #     for message_str in message_str_list:
    #         message_int, message_int_len = crypto_utils.ot_str_to_int(message_str)
    #         message_list.append(message_int)
    #     # message_list = [111111, 222222, 333333, 444444, 555555]
    #     cipher_vec = []
    #     for z_value in z_value_list:
    #         cipher_list = []
    #         for message in message_list:
    #             blinding_r = crypto_utils.get_random_int()
    #             blinding_s = crypto_utils.get_random_int()
    #             w_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(x_value, blinding_s),
    #                                             crypto_utils.ot_base_pown(blinding_r))
    #             key_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(z_value, blinding_s),
    #                                               crypto_utils.ot_pown(y_value, blinding_r))
    #             z_value = crypto_utils.ot_mul_n(z_value, crypto_utils.DEFAULT_G)
    #             e_cipher = key_value ^ message
    #             cipher_list.append({
    #                 "w": w_value,
    #                 "e": e_cipher
    #             })
    #         cipher_vec.append(cipher_list)
    #
    #     for cipher_each in cipher_vec:
    #         for cipher in cipher_each:
    #             w_1 = crypto_utils.ot_pown(cipher['w'], blinding_b)
    #             message_recover = w_1 ^ cipher['e']
    #             print(message_recover)
    #     for idx, cipher_list in enumerate(cipher_vec):
    #         w_1 = crypto_utils.ot_pown(cipher_list[choice_list[idx]]['w'], blinding_b)
    #         message_recover = w_1 ^ cipher_list[choice_list[idx]]['e']
    #         s = crypto_utils.ot_int_to_str(message_recover)
    #         print(s)

    # def test_id_ot(self):
    #     print("test_id_ot")
    #     choice_id_list = [crypto_utils.ot_str_to_int('小明'), crypto_utils.ot_str_to_int('张三')]
    #     blinding_a = crypto_utils.get_random_int()
    #     blinding_b = crypto_utils.get_random_int()
    #     x_value = crypto_utils.ot_base_pown(blinding_a)
    #     y_value = crypto_utils.ot_base_pown(blinding_b)
    #     c_blinding = crypto_utils.ot_mul_fi(blinding_a, blinding_b)
    #     # c_value = crypto_utils.ot_base_pown(c_blinding)
    #     z_value_list = []
    #     for choice in choice_id_list:
    #         z_value = crypto_utils.ot_base_pown(c_blinding - choice)
    #         z_value_list.append(z_value)
    #     # z_value = crypto_utils.ot_base_pown(c_blinding - choice_id)
    #     # send x_value, y_value, z_value
    #     id_str_list = ['小往', '小明', 'asher', 'set', '张三']
    #     id_list = []
    #     for id_str in id_str_list:
    #         id_list.append(crypto_utils.ot_str_to_int(id_str))
    #     message_str_list = ['hello', 'world', 'ot', 'cipher', 'test']
    #     message_list = []
    #     for message_str in message_str_list:
    #         message_list.append(crypto_utils.ot_str_to_int(message_str))
    #     # message_list = [111111, 222222, 333333, 444444, 555555]
    #     cipher_vec = []
    #     for z_value in z_value_list:
    #         cipher_list = []
    #         for idx, message in enumerate(message_list):
    #             blinding_r = crypto_utils.get_random_int()
    #             blinding_s = crypto_utils.get_random_int()
    #             z_value_use = crypto_utils.ot_mul_n(z_value, crypto_utils.ot_base_pown(id_list[idx]))
    #             w_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(x_value, blinding_s),
    #                                             crypto_utils.ot_base_pown(blinding_r))
    #             key_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(z_value_use, blinding_s),
    #                                               crypto_utils.ot_pown(y_value, blinding_r))
    #             e_cipher = key_value ^ message
    #             cipher_list.append({
    #                 "w": w_value,
    #                 "e": e_cipher
    #             })
    #         cipher_vec.append(cipher_list)
    #
    #     for idx, cipher_list in enumerate(cipher_vec):
    #         for now_idx, cipher in enumerate(cipher_list):
    #             w_1 = crypto_utils.ot_pown(cipher['w'], blinding_b)
    #             message_recover = w_1 ^ cipher['e']
    #             # print(message_recover)
    #             # print(idx)
    #             # print(now_idx)
    #             if (idx == 0 and now_idx == 1) or (idx == 1 and now_idx == 4):
    #                 s = crypto_utils.ot_int_to_str(message_recover)
    #                 print(s)

    def test_pd_with_multi_index(self):
        print(True)
        df = pd.DataFrame(
            [[21, 'Amol', 72, 67],
             [23, 'Lini', 78, 69],
             [32, 'Kiku', 74, 56],
             [52, 'Ajit', 54, 76],
             [53, 'Ajit', 55, 78]
             ],
            columns=['rollno', 'name', 'physics', 'botony'])

        print('DataFrame with default index\n', df)
        # set multiple columns as index
        # df_map = df.set_index('name').T.to_dict('list')
        # df_map = df.set_index('name').groupby(level=0).apply(lambda x: x.to_dict('r')).to_dict()
        df_map = df.set_index('name').groupby(level=0).apply(
            lambda x: x.to_dict('r')).to_dict()
        print(df_map)
        print(json.dumps(df_map['Ajit']))
        print(type(json.dumps(df_map['Kiku'])))

    def test_prepare_dataset_with_matrix(self):
        tmp_file_path = "/Users/asher/Desktop/数据集2021/8_1_100w.csv"
        prepare_dataset_tmp_file_path = "/Users/asher/Desktop/数据集2021/pre-test_100.csv"
        search_id_matrix = [['645515750175253924', '779808531920530393'], [
            '399352968694137676', '399352968694137676222', '399352968694137']]
        prepare_dataset_with_matrix(
            search_id_matrix, tmp_file_path, prepare_dataset_tmp_file_path)

    #
    # def test_choice_ot_multi(self):
    #     choice_list = [1, 2, 4]
    #     blinding_a = crypto_utils.get_random_int()
    #     blinding_b = crypto_utils.get_random_int()
    #     x_value = crypto_utils.ot_base_pown(blinding_a)
    #     y_value = crypto_utils.ot_base_pown(blinding_b)
    #     c_blinding = crypto_utils.ot_mul_fi(blinding_a, blinding_b)
    #     # c_value = crypto_utils.ot_base_pown(c_blinding)
    #     choice_final = 0
    #     for choice in choice_list:
    #         choice_final = choice_final + choice
    #     z_value = crypto_utils.ot_base_pown(c_blinding - choice_final)
    #     # send x_value, y_value, z_value
    #     message_str_list = ['hello', 'world', 'ot', 'cipher', 'test']
    #     message_list = []
    #     for message_str in message_str_list:
    #         message_list.append(crypto_utils.ot_str_to_int(message_str))
    #     # message_list = [111111, 222222, 333333, 444444, 555555]
    #     cipher_list = []
    #     for message in message_list:
    #         blinding_r = crypto_utils.get_random_int()
    #         blinding_s = crypto_utils.get_random_int()
    #         w_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(x_value, blinding_s),
    #                                         crypto_utils.ot_base_pown(blinding_r))
    #         key_value = crypto_utils.ot_mul_n(crypto_utils.ot_pown(z_value, blinding_s),
    #                                           crypto_utils.ot_pown(y_value, blinding_r))
    #         z_value = crypto_utils.ot_mul_n(z_value, crypto_utils.DEFAULT_G)
    #         e_cipher = key_value ^ message
    #         cipher_list.append({
    #             "w": w_value,
    #             "e": e_cipher
    #         })
    #
    #     # for cipher in cipher_list:
    #     #     w_1 = crypto_utils.ot_pown(cipher['w'], blinding_b)
    #     #     message_recover = w_1 ^ cipher['e']
    #     #     print(message_recover)
    #
    #     for choice in choice_list:
    #         w_1 = crypto_utils.ot_pown(cipher_list[choice]['w'], blinding_b)
    #         for item in choice_list:
    #             if choice == item:
    #                 continue
    #             else:
    #                 base_g = crypto_utils.ot_base_pown(-item)
    #                 w_1 = crypto_utils.ot_mul_n(w_1, base_g)
    #         message_recover = w_1 ^ cipher_list[choice]['e']
    #         print(message_recover)
    #         # s = crypto_utils.ot_int_to_str(message_recover)
    #         # print(s)
    #


# if __name__ == '__main__':

    # print(True)
    # df = pd.DataFrame(
    #     [[21, 'Amol', 72, 67],
    #      [23, 'Lini', 78, 69],
    #      [32, 'Kiku', 74, 56],
    #      [52, 'Ajit', 54, 76]],
    #     columns=['rollno', 'name', 'physics', 'botony'])
    #
    # print('DataFrame with default index\n', df)
    # # set multiple columns as index
    # df = df.set_index(['rollno', 'name'])
    #
    # print('\nDataFrame with MultiIndex\n', df)
    # point1 = point.base()
    # print(point.base(scalar1).hex())

    # json_response = "{\"id\": {\"0\": \"67b176705b46206614219f47a05aee7ae6a3edbe850bbbe214c536b989aea4d2\", \"1\": \"b1b1bd1ed240b1496c81ccf19ceccf2af6fd24fac10ae42023628abbe2687310\"}, \"x0\": {\"0\": 10, \"1\": 20}, \"x1\": {\"0\": 11, \"1\": 22}}"
    # json_dict = json.loads(json_response)
    # # json_pd = pd.json_normalize(json_dict)
    # print(json_dict)

    # hex_str = utils.make_hash(bytes(str(796443), 'utf8'), CryptoType.ECDSA, HashType.HEXSTR)
    # print(hex_str)
    # csv_path1 = '/Users/asher/Downloads/UseCase120/usecase120_party1.csv'
    # csv_path2 = '/Users/asher/Downloads/UseCase120/usecase120_party2.csv'
    # data = pd.read_csv(csv_path1)
    # if 'id' in data.columns.values:
    #     duplicated_list = data.duplicated('id', False).tolist()
    #     if True in duplicated_list:
        # log.error(f"[OnError]id duplicated, check csv file")
        # raise PpcException(PpcErrorCode.DATASET_CSV_ERROR.get_code(),
        #                    PpcErrorCode.DATASET_CSV_ERROR.get_msg())
    # start = time.time()
    # get_pd_file_with_hash_requester(csv_path1, f'{csv_path1}-pre', 2)
    # start2 = time.time()
    # print(f"requester prepare time is {start2 - start}s")
    #
    # get_pd_file_with_hash_data_provider(csv_path2, f'{csv_path2}-pre')
    # start3 = time.time()
    # print(f"provider prepare time is {start3 - start2}s")
    # provider_output_path = '/Users/asher/Downloads/UseCase120/output.csv'
    # get_anonymous_data(f'{csv_path1}-pre-requester', f'{csv_path2}-pre', provider_output_path)
    # start4 = time.time()
    # print(f"provider compute time is {start4 - start3}s")
    # result_path = '/Users/asher/Downloads/UseCase120/result.csv'
    # recover_result_data(f'{csv_path1}-pre', '/Users/asher/Downloads/UseCase120/output.csv', result_path)
    # end = time.time()
    # print(f"requester get result time is {end - start4}s")
