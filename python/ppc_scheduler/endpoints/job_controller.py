# -*- coding: utf-8 -*-

from flask import request
from flask_restx import Resource

from ppc_common.ppc_utils import utils
from ppc_common.ppc_utils.exception import PpcErrorCode, PpcException
from ppc_scheduler.common.global_context import components
from ppc_scheduler.endpoints.body_schema import response_job_status, response_base
from ppc_scheduler.endpoints.restx import api
from ppc_scheduler.common.global_job_manager import get_job_manager

ns = api.namespace('/wedpr/v3/scheduler/job',
                   description='Operations related to run job')


@ns.route('/<string:job_id>')
class JobCollection(Resource):

    @api.response(200, 'Job started successfully.', response_base)
    def post(self, job_id):
        """
        Run a specific job by job_id.
        """
        request_body = request.get_json()
        
        if 'jobId' not in request_body:
            raise PpcException(
                    PpcErrorCode.PARAMETER_CHECK_ERROR.get_code(),
                    f"Missing 'jobId' in request")
        # if 'agency' not in request_body:
        #     raise PpcException(
        #             PpcErrorCode.PARAMETER_CHECK_ERROR.get_code(),
        #             f"Missing 'agency' in request")
        if 'workflow' not in request_body:
            raise PpcException(
                    PpcErrorCode.PARAMETER_CHECK_ERROR.get_code(),
                    f"Missing 'workflow' in request")
        
        components.logger().info(f"Recv run job request, job_id: {job_id}, request: {request_body}")
        
        get_job_manager().run_task(job_id, request_body)
        return utils.BASE_RESPONSE

    @api.response(200, 'Job status retrieved successfully.', response_job_status)
    def get(self, job_id):
        """
        Get the status of a specific job by job_id.
        """
        response = utils.BASE_RESPONSE
        
        components.logger().info(f"Recv query job request, job_id: {job_id}")
        
        status, time_costs = get_job_manager().status(job_id)
        response['data'] = {
            'status': status,
            'time_costs': time_costs
        }
        return response

    @api.response(200, 'Job killed successfully.', response_base)
    def delete(self, job_id):
        """
        Kill a specific job by job_id.
        """
        
        components.logger().info(f"receive kill job request, job_id: {job_id}")
        
        get_job_manager().kill_job(job_id)
        return utils.BASE_RESPONSE
