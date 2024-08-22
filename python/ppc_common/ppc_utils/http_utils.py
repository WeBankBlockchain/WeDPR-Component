# -*- coding: utf-8 -*-
import json
import logging
from json.decoder import JSONDecodeError

import requests
import urllib3
from urllib3.exceptions import SecurityWarning

from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode

log = logging.getLogger(__name__)


def check_response(response):
    if response.status_code != 200 and response.status_code != 201:
        message = response.text
        raise PpcException(PpcErrorCode.NETWORK_ERROR,
                           f"Call request failed, response message:{message}")


def send_get_request(endpoint, uri, params=None, headers=None):
    urllib3.disable_warnings(SecurityWarning)
    if not headers:
        headers = {'content-type': 'application/json'}
    if uri:
        url = f"http://{endpoint}{uri}"
    else:
        url = f"http://{endpoint}"
    log.debug(f"send a get request, url: {url}, params: {params}")
    response = requests.get(url=url, params=params,
                            headers=headers, timeout=30)
    log.debug(f"response: {response.text}")
    check_response(response)
    response_data = json.loads(response.text)

    return response_data


def send_post_request(endpoint, uri, params=None, headers=None, data=None):
    if not headers:
        headers = {'content-type': 'application/json'}
    if uri:
        url = f"http://{endpoint}{uri}"
    else:
        url = f"http://{endpoint}"
    log.debug(f"send a post request, url: {url}, params: {params}")
    response = requests.post(url, data=data, json=params, headers=headers)
    log.debug(f"response: {response.text}")
    # check_response(response)
    try:
        response_data = json.loads(response.text)
    except JSONDecodeError:
        response_data = response.text
    return response_data


def send_delete_request(endpoint, uri, params=None, headers=None):
    if not headers:
        headers = {'content-type': 'application/json'}
    if uri:
        url = f"http://{endpoint}{uri}"
    else:
        url = f"http://{endpoint}"
    log.debug(f"send a delete request, url: {url}, params: {params}")
    response = requests.delete(url, json=params, headers=headers)
    check_response(response)
    log.debug(f"response: {response.text}")
    response_data = json.loads(response.text)

    return response_data


def send_patch_request(endpoint, uri, params=None, headers=None, data=None):
    if not headers:
        headers = {'content-type': 'application/json'}
    url = f"http://{endpoint}{uri}"
    log.debug(f"send a patch request, url: {url}, params: {params}")
    response = requests.patch(url, data=data, json=params, headers=headers)
    check_response(response)
    log.debug(f"response: {response.text}")
    response_data = json.loads(response.text)

    return response_data


def send_upload_request(endpoint, uri, params=None, headers=None, data=None):
    if not headers:
        headers = {'content-type': 'application/json'}
    if uri:
        url = f"http://{endpoint}{uri}"
    else:
        url = endpoint
    log.debug(f"send a post request, url: {url}, params: {params}")
    response = requests.post(url, data=data, json=params, headers=headers)
    log.debug(f"response: {response.text}")
    check_response(response)
    try:
        response_data = json.loads(response.text)
    except JSONDecodeError:
        response_data = response.text
    return response_data
