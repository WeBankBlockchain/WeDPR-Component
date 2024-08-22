# -*- coding: utf-8 -*-

from flask_restx import Api

from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_model.common.global_context import components

authorizations = {
    'apikey': {
        'type': 'apiKey',
        'in': 'header',
        'name': 'Authorization'
    }
}

api = Api(version='1.0', title='Ppc Model Service',
          authorizations=authorizations, security='apikey')


@api.errorhandler(PpcException)
def default_error_handler(e):
    components.logger().exception(e)
    info = e.to_dict()
    response = {'errorCode': info['code'], 'message': info['message']}
    components.logger().error(
        f"OnError: code: {info['code']}, message: {info['message']}")
    return response, 500


@api.errorhandler(BaseException)
def default_error_handler(e):
    components.logger().exception(e)
    message = 'unknown error.'
    response = {'errorCode': PpcErrorCode.INTERNAL_ERROR, 'message': message}
    components.logger().error(f"OnError: unknown error")
    return response, 500
