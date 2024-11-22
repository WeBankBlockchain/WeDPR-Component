# -*- coding: utf-8 -*-

from flask import request
from flask_restx import Resource
import time

from ppc_common.ppc_utils import utils
from ppc_model.common.global_context import components
from ppc_model.common.protocol import ModelTask
from ppc_model.network.http.body_schema import response_task_status, response_base
from ppc_model.network.http.restx import api
from ppc_model.model_result.task_result_handler import TaskResultHandler
from ppc_model.model_result.task_result_handler import TaskResultRequest

ns = api.namespace('ppc-model/pml/run-model-task',
                   description='Operations related to run model task')
ns2 = api.namespace('ppc-model/pml/record-model-log',
                    description='Operations related to record model log')
ns_get_job_result = api.namespace(
    'ppc-model/pml/get-job-result', description='Get the job result')


@ns.route('/<string:model_id>')
class ModelCollection(Resource):

    @api.response(201, 'Task started successfully.', response_base)
    def post(self, model_id):
        """
        Run a specific task by task_id.
        """
        try:
            args = request.get_json()
            task_id = model_id
            components.logger().info(
                f"run task request, task_id: {task_id}")
            task_type = args['task_type']
            components.task_manager.run_task(
                task_id, ModelTask(task_type), (args,))
            return utils.BASE_RESPONSE
        except Exception as e:
            response = {}
            response.update({'errorCode': -1})
            response.update(
                {'message': f'submit task {model_id} failed for {e}'})
            return response

    @api.response(200, 'Task status retrieved successfully.', response_task_status)
    def get(self, model_id):
        try:
            """
            Get the status of a specific task by task_id.
            """
            response = utils.BASE_RESPONSE
            task_id = model_id
            status, traffic_volume, exec_result = components.task_manager.status(
                task_id)
            response['data'] = {
                'status': status,
                'traffic_volume': traffic_volume,
                'exec_result': exec_result,
            }
            return response
        except Exception as e:
            response = {}
            response.update({'errorCode': -1})
            response.update(
                {'message': f'query task status for {model_id} failed for {e}'})
            return response

    @api.response(200, 'Task killed successfully.', response_base)
    def delete(self, model_id):
        try:
            """
            Kill a specific task by job_id.
            """
            job_id = model_id
            components.logger().info(f"kill request, job_id: {job_id}")
            components.task_manager.kill_task(job_id)
            return utils.BASE_RESPONSE
        except Exception as e:
            response = {}
            response.update({'errorCode': -1})
            response.update(
                {'message': f'kill task {model_id} failed for {e}'})
            return response


@ns_get_job_result.route('/<string:task_id>')
class ModelResultCollection(Resource):
    @api.response(201, 'Get task result successfully.', response_base)
    def post(self, task_id):
        try:
            """
            Get the result related to the task_id
            """
            start_t = time.time()
            args = request.get_json()
            components.logger().info(
                f"get task result, task_id: {task_id}, args: {args}")
            user_name = args['user']
            task_type = args['jobType']
            only_fetch_log = {'True': True, 'False': False}.get(
                args['onlyFetchLog'])
            components.logger().info(
                f"get_job_direct_result_response, job: {task_id}")
            task_result_request = TaskResultRequest(
                task_id, task_type, only_fetch_log)
            job_result_handler = TaskResultHandler(
                task_result_request=task_result_request, components=components)
            response = job_result_handler.get_response()
            components.logger().info(
                f"get_job_direct_result_response success, user: {user_name}, job: {task_id}, timecost: {time.time() - start_t}s")
            return response
        except Exception as e:
            response = {}
            response.update({'errorCode': -1})
            response.update(
                {'message': f'query task log for {model_id} failed for {e}'})
            return response
