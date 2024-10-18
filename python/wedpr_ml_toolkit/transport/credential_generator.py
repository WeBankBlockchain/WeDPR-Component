# -*- coding: utf-8 -*-
import hashlib
from wedpr_ml_toolkit.common import utils
import time


class CredentialInfo:
    ACCESS_ID_KEY = "accessKeyID"
    NONCE_KEY = "nonce"
    TIMESTAMP_KEY = "timestamp"
    SIGNATURE_KEY = "signature"

    def __init__(self, access_key_id: str, nonce: str, timestamp: str, signature: str):
        self.access_key_id = access_key_id
        self.nonce = nonce
        self.timestamp = timestamp
        self.signature = signature

    def to_dict(self):
        result = {}
        result.update(CredentialInfo.ACCESS_ID_KEY, self.access_key_id)
        result.update(CredentialInfo.NONCE_KEY, self.nonce)
        result.update(CredentialInfo.TIMESTAMP_KEY, self.timestamp)
        result.update(CredentialInfo.SIGNATURE_KEY, self.signature)

    def update_url_with_auth_info(self, url):
        auth_params = self.to_dict()
        return utils.add_params_to_url(auth_params)


class CredentialGenerator:
    def __init__(self, access_key_id: str, access_key_secret: str, nonce_len=5):
        self.access_key_id = access_key_id
        self.access_key_secret = access_key_secret
        self.nonce_len = nonce_len

    def generate_credential(self) -> CredentialInfo:
        nonce = utils.generate_nonce(self.nonce_len)
        timestamp = int(time.time())
        # generate the signature
        signature = CredentialGenerator.generate_signature(
            self.access_key_id, self.access_key_secret, nonce, timestamp)
        return CredentialInfo(self.access_key_id, nonce, timestamp, signature)

    @staticmethod
    def generate_signature(access_key_id, access_key_secret, nonce, timestamp) -> str:
        anti_replay_info_hash = hashlib.sha3_256()
        # hash(access_key_id + nonce + timestamp)
        anti_replay_info = f"{access_key_id}{nonce}{timestamp}"
        anti_replay_info_hash.update(anti_replay_info)
        # hash(anti_replay_info + access_key_secret)
        signature_hash = hashlib.sha3_256()
        signature_hash.update(anti_replay_info_hash.hexdigest())
        signature_hash.update(access_key_secret)
        return signature_hash.hexdigest()
