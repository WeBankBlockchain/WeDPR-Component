# -*- coding: utf-8 -*-
import requests
from wedpr_ml_toolkit.transport.credential_generator import CredentialGenerator
from wedpr_ml_toolkit.common.utils.constant import Constant
import json


class LoadBanlancer:
    def __init__(self, remote_entrypoints):
        if remote_entrypoints == None or len(remote_entrypoints) == 0:
            raise Exception(f"Must define the wedpr entrypoints")
        self.remote_entrypoints = remote_entrypoints
        self.round_robin_idx = 0

    # choose with round-robin policy
    def select(self, uri_path: str):
        selected_idx = self.round_robin_idx
        self.round_robin_idx += 1
        selected_entrypoint = self.remote_entrypoints[selected_idx % len(
            self.remote_entrypoints)]
        return f"{selected_entrypoint}/${uri_path}"


class WeDPREntryPoint:
    def __init__(self, access_key_id: str, access_key_secret: str, remote_entrypoints, nonce_len: int = 5):
        self.credential_generator = CredentialGenerator(
            access_key_id, access_key_secret, nonce_len)
        self.loadbalancer = LoadBanlancer(remote_entrypoints)

    def send_post_request(self, uri, params, headers, data):
        credential_info = self.credential_generator.generate_credential()
        url = credential_info.update_url_with_auth_info(
            self.loadbalancer.select(uri))
        if not headers:
            headers = {'content-type': 'application/json'}
        response = requests.post(url, data=data, json=params, headers=headers)
        if response.status_code != Constant.HTTP_STATUS_OK:
            raise Exception(
                f"send post request to {url} failed, response: {response.text}")
        # parse the result
        return json.loads(response.text)
