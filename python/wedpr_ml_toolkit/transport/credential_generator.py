# -*- coding: utf-8 -*-
import hashlib
from wedpr_ml_toolkit.common.utils import utils
from datetime import datetime


class CredentialInfo:
    ACCESS_ID_KEY = "accessKeyID"
    NONCE_KEY = "nonce"
    TIMESTAMP_KEY = "timestamp"
    SIGNATURE_KEY = "signature"
    HASH_ALGORITHM_KEY = "hashAlgorithm"

    def __init__(self, access_key_id: str, nonce: str, timestamp: int, signature: str):
        self.access_key_id = access_key_id
        self.nonce = nonce
        self.timestamp = timestamp
        self.signature = signature
        # use SHA3-256 algorithm
        self.hash_algorithm = "SHA3-256"

    def to_dict(self):
        result = {}
        result.update({CredentialInfo.ACCESS_ID_KEY: self.access_key_id})
        result.update({CredentialInfo.NONCE_KEY: self.nonce})
        result.update({CredentialInfo.TIMESTAMP_KEY: self.timestamp})
        result.update({CredentialInfo.SIGNATURE_KEY: self.signature})
        result.update({CredentialInfo.HASH_ALGORITHM_KEY: self.hash_algorithm})
        return result


class CredentialGenerator:
    def __init__(self, access_key_id: str, access_key_secret: str, nonce_len=5):
        self.access_key_id = access_key_id
        self.access_key_secret = access_key_secret
        self.nonce_len = nonce_len

    def generate_credential(self) -> CredentialInfo:
        nonce = utils.generate_nonce(self.nonce_len)
        # convert to the million-seconds timestamp
        timestamp = int(datetime.now().timestamp() * 1000)
        # generate the signature
        signature = CredentialGenerator.generate_signature(
            self.access_key_id, self.access_key_secret, nonce, timestamp)
        return CredentialInfo(self.access_key_id, nonce, timestamp, signature)

    @staticmethod
    def generate_signature(access_key_id, access_key_secret, nonce, timestamp) -> str:
        anti_replay_info_hash = hashlib.sha3_256()
        # hash(access_key_id + nonce + timestamp)
        anti_replay_info_hash.update(
            bytes(access_key_id + nonce + str(timestamp), encoding='utf-8'))
        # hash(anti_replay_info + access_key_secret)
        signature_hash = hashlib.sha3_256()
        signature_hash.update(
            bytes(anti_replay_info_hash.hexdigest(), encoding='utf-8'))
        signature_hash.update(bytes(access_key_secret, encoding='utf-8'))
        return signature_hash.hexdigest()
