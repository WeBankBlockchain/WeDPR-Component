from enum import Enum

from ppc_common.ppc_protos.generated.ppc_model_pb2 import Cipher1DimList, Cipher2DimList
from ppc_common.ppc_protos.generated.ppc_model_pb2 import CipherList, ModelCipher
from ppc_common.ppc_utils import utils


class TaskRole(Enum):
    ACTIVE_PARTY = "ACTIVE_PARTY"
    PASSIVE_PARTY = "PASSIVE_PARTY"


class ModelTask(Enum):
    PREPROCESSING = "PREPROCESSING"
    FEATURE_ENGINEERING = "FEATURE_ENGINEERING"
    XGB_TRAINING = "XGB_TRAINING"
    XGB_PREDICTING = "XGB_PREDICTING"
    LR_TRAINING = "LR_TRAINING"
    LR_PREDICTING = "LR_PREDICTING"


class TaskStatus(Enum):
    NotFound = "NotFound"
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    FAILURE = "FAILURE"
    KILLED = "KILLED"
    SUCCESS = "SUCCESS"


class RpcType(Enum):
    HTTP = "HTTP"
    GRPC = "GRPC"


class PheMessage:

    @staticmethod
    def packing_data(codec, public_key, cipher_list):
        enc_data_pb = CipherList()
        enc_data_pb.public_key = codec.encode_enc_key(public_key)

        for cipher in cipher_list:
            model_cipher = ModelCipher()
            model_cipher.ciphertext, model_cipher.exponent = codec.encode_cipher(
                cipher)
            enc_data_pb.cipher_list.append(model_cipher)

        return utils.pb_to_bytes(enc_data_pb)

    @staticmethod
    def unpacking_data(codec, data):
        enc_data_pb = CipherList()
        utils.bytes_to_pb(enc_data_pb, data)
        public_key = codec.decode_enc_key(enc_data_pb.public_key)
        enc_data = [codec.decode_cipher(public_key,
                                        cipher.ciphertext,
                                        cipher.exponent
                                        ) for cipher in enc_data_pb.cipher_list]
        return public_key, enc_data

    @staticmethod
    def packing_2dim_data(codec, public_key, cipher_2d_list):
        enc_data_pb = Cipher2DimList()
        enc_data_pb.public_key = codec.encode_enc_key(public_key)

        for cipher_list in cipher_2d_list:
            enc_1d_pb = Cipher1DimList()
            for cipher in cipher_list:
                model_cipher = ModelCipher()
                model_cipher.ciphertext, model_cipher.exponent = \
                    codec.encode_cipher(cipher, be_secure=False)
                enc_1d_pb.cipher_list.append(model_cipher)
            enc_data_pb.cipher_1d_list.append(enc_1d_pb)

        return utils.pb_to_bytes(enc_data_pb)

    @staticmethod
    def unpacking_2dim_data(codec, data):
        enc_data_pb = Cipher2DimList()
        utils.bytes_to_pb(enc_data_pb, data)
        public_key = codec.decode_enc_key(enc_data_pb.public_key)
        enc_data = []
        for enc_1d_pb in enc_data_pb.cipher_1d_list:
            enc_1d_data = [codec.decode_cipher(public_key,
                                               cipher.ciphertext,
                                               cipher.exponent
                                               ) for cipher in enc_1d_pb.cipher_list]
            enc_data.append(enc_1d_data)
        return public_key, enc_data


LOG_START_FLAG_FORMATTER = "$$$StartModelJob:{job_id}"
LOG_END_FLAG_FORMATTER = "$$$EndModelJob:{job_id}"
