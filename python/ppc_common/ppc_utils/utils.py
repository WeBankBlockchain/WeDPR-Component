import base64
import hashlib
import io
import json
import logging
import os
import re
import shutil
import subprocess
import time
from enum import Enum, unique

import jwt
from ecdsa import SigningKey, SECP256k1, VerifyingKey
from gmssl import func, sm2, sm3
from google.protobuf.descriptor import FieldDescriptor
from jsoncomment import JsonComment
from pysmx.SM3 import SM3

from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode

log = logging.getLogger(__name__)

MAX_SUPPORTED_PARTIES = 5

SERVER_RUNNING_STATUS = 0
DEFAULT_DATASET_RECORD_COUNT = 5
DEFAULT_PAGE_OFFSET = 0
DEFAULT_PAGE_SIZE = 5
MIN_PARTICIPATE_NUMBER = 2
TWO_PARTY_PSI_PARTICIPATE_NUMBER = 2
MIN_MULTI_PARTY_PSI_PARTICIPATE_NUMBER = 3
MPC_MAX_SOURCE_COUNT = 50
MPC_MAX_FIELD_COUNT = 50

BASE_RESPONSE = {'errorCode': PpcErrorCode.SUCCESS.get_code(
), 'message': PpcErrorCode.SUCCESS.get_msg()}
LOG_NAME = 'ppcs-modeladm-scheduler.log'
LOG_CHARACTER_NUMBER = 100000
CSV_SEP = ','
BLANK_SEP = ' '
NORMALIZED_NAMES = 'field{}'

PPC_RESULT_FIELDS_FLAG = 'result_fields'
PPC_RESULT_VALUES_FLAG = 'result_values'

ADMIN_USER = 'admin'

MPC_RECORD_PLACE_HOLDER = '$(ppc_max_record_count)'
MPC_START_PLACE_HOLDER = '${ph_start}'
MPC_END_PLACE_HOLDER = '${ph_end}'

HOMO_MODEL_ALGORITHM = 'homo'
MPC_TOTAL_RECORD_COUNT_PLACE_HOLDER = '$(total_record_count)'
MPC_TRAIN_RECORD_COUNT_PLACE_HOLDER = '$(train_record_count)'
MPC_TEST_RECORD_COUNT_PLACE_HOLDER = '$(test_record_count)'
MPC_TOTAL_FEATURE_COUNT_PLACE_HOLDER = '$(total_feature_count)'

XGB_TREE_PERFIX = "xgb_tree"
MPC_TRAIN_METRIC_ROC_FILE = "mpc_metric_roc.svg"
MPC_TRAIN_METRIC_KS_FILE = "mpc_metric_ks.svg"
MPC_TRAIN_METRIC_PR_FILE = "mpc_metric_pr.svg"
MPC_TRAIN_METRIC_ACCURACY_FILE = "mpc_metric_accuracy.svg"
MPC_TRAIN_METRIC_KS_TABLE = "mpc_metric_ks.csv"
MPC_TRAIN_SET_METRIC_ROC_FILE = "mpc_train_metric_roc.svg"
MPC_TRAIN_SET_METRIC_KS_FILE = "mpc_train_metric_ks.svg"
MPC_TRAIN_SET_METRIC_PR_FILE = "mpc_train_metric_pr.svg"
MPC_TRAIN_SET_METRIC_ACCURACY_FILE = "mpc_train_metric_accuracy.svg"
MPC_TRAIN_SET_METRIC_KS_TABLE = "mpc_train_metric_ks.csv"
MPC_TRAIN_METRIC_CONFUSION_MATRIX_FILE = "mpc_metric_confusion_matrix.svg"
METRICS_OVER_ITERATION_FILE = "metrics_over_iterations.svg"

# the ks-auc table, e.g.:
#      |总样本|正样本|  KS    |  AUC   |
# 训练集| 500 | 100 | 0.4161 | 0.7685 |
# 验证集| 154 | 37  | 0.2897 | 0.6376 |
MPC_XGB_EVALUATION_TABLE = "mpc_xgb_evaluation_table.csv"

# the feature-importance table, e.g.:
# |特征 | score | weight | score_rank| topk |
# | x1 | 0.08  |  1000   | 1        |      |
# | x2 | 0.07  |  900    | 2        |      |
XGB_FEATURE_IMPORTANCE_TABLE = "xgb_result_feature_importance_table.csv"

# png, jpeg, pdf etc. ref: https://graphviz.org/docs/outputs/
WORKFLOW_VIEW_FORMAT = 'svg'

WORKFLOW_VIEW_NAME = 'workflow_view'

PROXY_PSI_CIPHER_SUITE = "HMAC_BASED_PRIVATE_SET_INTERSECTION"
PROXY_MPC_CIPHER_SUITE = "SHA256_WITH_REPLICATED_SECRET_SHARING"
PROXY_PSI_MPC_CIPHER_SUITE = PROXY_PSI_CIPHER_SUITE + "-" + PROXY_MPC_CIPHER_SUITE
PPC_ALL_AUTH_FLAG = "PPC_ALGO_ALL"

CEM_CIPHER_LEN = 288


@unique
class PPCModleType(Enum):
    Train = 1
    Predict = 2


@unique
class CryptoType(Enum):
    ECDSA = 1
    GM = 2


@unique
class HashType(Enum):
    BYTES = 1
    HEXSTR = 2


class AlgorithmType(Enum):
    Train = "Train",
    Predict = "Predict"


def json_loads(json_config):
    try:
        json_comment = JsonComment(json)
        return json_comment.loads(json_config)
    except BaseException:
        raise PpcException(PpcErrorCode.ALGORITHM_PPC_CONFIG_ERROR.get_code(),
                           PpcErrorCode.ALGORITHM_PPC_CONFIG_ERROR.get_msg())


def parse_n_class(layer_str):
    if layer_str == '[]':
        return 1
    else:
        return int(re.findall(r"\d+\.?\d*", layer_str)[-1])


def check_ppc_model_algorithm_is_homo(algorithm_name):
    return algorithm_name[0:4].lower() == HOMO_MODEL_ALGORITHM


def get_log_file_path(app_dir):
    return os.sep.join([app_dir, "logs", LOG_NAME])


def get_log_temp_file_path(app_dir, job_id):
    return os.sep.join([app_dir, "logs", f"{job_id}.log"])


def df_to_dict(df, orient='split'):
    data_json_str = df.to_json(orient=orient, force_ascii=False)
    data_dict = json.loads(data_json_str)
    del data_dict['index']
    return data_dict


def file_exists(_file):
    if os.path.exists(_file) and os.path.isfile(_file):
        return True
    return False


def decode_jwt(token):
    """
    decode jwt
    :param token:
    :return:
    """
    result = {"data": None, "error": None}
    try:
        payload = jwt.decode(token.split(" ")[1], options={
                             "verify_signature": False})
        result["data"] = payload
    except (IndexError, jwt.DecodeError):
        result["error"] = "JWT token is decoded fail"
    return result


def make_timestamp():
    return int(round(time.time() * 1000))


def encode(data_bytes):
    return base64.b64encode(data_bytes)


def decode(data_str):
    return base64.b64decode(data_str)


def pb_to_str(data_pb):
    return encode(data_pb.SerializeToString()).decode("utf-8")


def str_to_pb(data_pb, data_str):
    data_pb.ParseFromString(decode(data_str))
    return data_pb


def pb_to_bytes(data_pb):
    return data_pb.SerializeToString()


def bytes_to_pb(data_pb, data_bytes):
    return data_pb.ParseFromString(data_bytes)


def str_to_base64str(data_str):
    message_bytes = data_str.encode('utf-8')
    base64_bytes = base64.b64encode(message_bytes)
    base64_message = base64_bytes.decode('utf-8')
    return base64_message


def base64str_to_str(base64_str):
    base64_bytes = base64_str.encode('utf-8')
    message_bytes = base64.b64decode(base64_bytes)
    message = message_bytes.decode('utf-8')
    return message


def bytes_to_base64str(data_bytes):
    base64_bytes = base64.b64encode(data_bytes)
    base64_message = base64_bytes.decode('utf-8')
    return base64_message


def make_response(code, message, data=None):
    return {'errorCode': code, 'message': message, 'data': data}


def base64str_to_bytes(base64_str):
    base64_bytes = base64_str.encode('utf-8')
    message_bytes = base64.b64decode(base64_bytes)
    return message_bytes


def make_hash_from_file_path(file_path, crypto_type):
    file_data = read_content_from_file_by_binary(file_path)
    return make_hash(file_data, crypto_type, HashType.HEXSTR)


def read_chunks(file, size=io.DEFAULT_BUFFER_SIZE):
    while True:
        chunk = bytes(file.read(size), 'utf-8')
        if not chunk:
            break
        yield chunk


def make_hash_from_file_path_by_chunks(file_path, crypto_type, block_size=1 << 10):
    m = SM3()
    if crypto_type == CryptoType.ECDSA:
        m = hashlib.sha3_256()
    dataset_size = 0
    with open(file_path, 'r', encoding='utf-8') as f:
        for block in read_chunks(f, size=block_size):
            dataset_size += len(block)
            m.update(block)
    return m.hexdigest(), dataset_size


def read_content_from_file_by_binary(file_path):
    with open(file_path, 'rb') as file:
        content = file.read()
    return content


def make_hash(data, crypto_type, hash_type=None):
    if crypto_type == CryptoType.ECDSA:
        m = hashlib.sha3_256()
        m.update(data)
        if hash_type == HashType.HEXSTR:
            return m.hexdigest()
        if hash_type == HashType.BYTES:
            return m.digest()
    if crypto_type == CryptoType.GM:
        return sm3.sm3_hash(func.bytes_to_list(data))


def pb2dict(obj):
    """
    Takes a ProtoBuf Message obj and convertes it to a dict.
    """
    adict = {}
    if not obj.IsInitialized():
        return None
    for field in obj.DESCRIPTOR.fields:
        if not getattr(obj, field.name):
            continue
        if not field.label == FieldDescriptor.LABEL_REPEATED:
            if not field.type == FieldDescriptor.TYPE_MESSAGE:
                adict[field.name] = getattr(obj, field.name)
            else:
                value = pb2dict(getattr(obj, field.name))
                if value:
                    adict[field.name] = value
        else:
            if field.type == FieldDescriptor.TYPE_MESSAGE:
                adict[field.name] = \
                    [pb2dict(v) for v in getattr(obj, field.name)]
            else:
                adict[field.name] = [v for v in getattr(obj, field.name)]
    return adict


def write_content_to_file(content, file_path):
    with open(file_path, 'w', encoding="utf-8") as file:
        file.write(content)


def write_content_to_file_by_append(content, file_path):
    with open(file_path, 'a', encoding="utf-8") as file:
        file.write(content)


def read_content_from_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()
    return content


def make_job_event_message(job_id, job_priority, initiator_agency_id, receiver_agency_id, job_algorithm_id,
                           job_dataset):
    message = '{}|{}|{}|{}|{}'.format(job_id, job_priority, initiator_agency_id, receiver_agency_id, job_algorithm_id,
                                      job_dataset)
    return message.encode('utf-8')


def sign_with_secp256k1(message, private_key_str):
    if isinstance(private_key_str, str):
        private_key = SigningKey.from_string(
            bytes().fromhex(private_key_str), curve=SECP256k1)
    else:
        private_key = SigningKey.from_string(private_key_str, curve=SECP256k1)
    signature_bytes = private_key.sign(
        make_hash(message, CryptoType.ECDSA, HashType.BYTES))
    return str(encode(signature_bytes), 'utf-8')


def verify_with_secp256k1(message, signature_str, public_key_str):
    if isinstance(public_key_str, str):
        verify_key = VerifyingKey.from_string(
            bytes().fromhex(public_key_str), curve=SECP256k1)
    else:
        verify_key = VerifyingKey.from_string(
            decode(public_key_str), curve=SECP256k1)
    return verify_key.verify(decode(signature_str), make_hash(message, CryptoType.ECDSA, HashType.BYTES))


def sign_with_sm2(message, private_key_str):
    sm2_crypt = sm2.CryptSM2(private_key_str, "")
    random_hex_str = func.random_hex(sm2_crypt.para_len)
    message_hash = make_hash(message, CryptoType.GM).encode(encoding='utf-8')
    signature_str = sm2_crypt.sign(message_hash, random_hex_str)
    return signature_str


def verify_with_sm2(message, signature_str, public_key_str):
    sm2_crypt = sm2.CryptSM2("", public_key_str)
    message_hash = make_hash(message, CryptoType.GM).encode(encoding='utf-8')
    return sm2_crypt.verify(signature_str, message_hash)


def make_signature(message, private_key_str, crypto_type):
    if crypto_type == CryptoType.ECDSA:
        return sign_with_secp256k1(message, private_key_str)

    if crypto_type == CryptoType.GM:
        return sign_with_sm2(message, private_key_str)


def verify_signature(message, signature, public_key_str, crypto_type):
    if crypto_type == CryptoType.ECDSA:
        return verify_with_secp256k1(message, signature, public_key_str)

    if crypto_type == CryptoType.GM:
        return verify_with_sm2(message, signature, public_key_str)


def exec_bash_command(cmd):
    """replace commands.get_status_output

    Arguments:
        cmd {[string]}
    """

    get_cmd = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    ret = get_cmd.communicate()
    out = ret[0]
    err = ret[1]
    output = ''
    if out is not None:
        output = output + out.decode('utf-8')
    if err is not None:
        output = output + err.decode('utf-8')

    return get_cmd.returncode, output


def delete_file(path):
    """[delete data_dir]

    Arguments:
        path {[get_dir]} -- [description]
    """

    if os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path)
    else:
        raise (Exception(' path not exisited ! path => %s', path))


def make_dir(_dir):
    if not os.path.exists(_dir):
        os.mkdir(_dir)


def load_credential_from_file(filepath):
    real_path = os.path.join(os.path.dirname(__file__), filepath)
    with open(real_path, 'rb') as f:
        return f.read()


def merge_files(file_list, output_file):
    try:
        with open(output_file, 'wb') as outfile:
            for file_name in file_list:
                with open(file_name, 'rb') as f:
                    outfile.write(f.read())
    except Exception as pe:
        log.info(f"merge files failed:  {pe}")
        raise PpcException(-1, f"merge files failed for: {pe}")


def md5sum(data_content):
    md5_hash = hashlib.md5()
    data = data_content if type(
        data_content) is bytes else bytes(data_content, "utf-8")
    md5_hash.update(data)
    return md5_hash.hexdigest()


def calculate_md5(file_path, granularity=2 * 1024 * 1024):
    md5_hash = hashlib.md5()

    with open(file_path, 'rb') as file:
        # 逐块读取文件内容，以提高性能
        for chunk in iter(lambda: file.read(granularity), b''):
            md5_hash.update(chunk)

    return md5_hash.hexdigest()
