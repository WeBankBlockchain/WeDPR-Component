# -*- coding: utf-8 -*-
from datetime import datetime
import json
from ppc_common.db_models.job_worker_record import JobWorkerRecord
from ppc_scheduler.common.global_context import components
from ppc_scheduler.database import job_worker_mapper
from ppc_scheduler.workflow.common import flow_utils
from ppc_scheduler.workflow.common.worker_status import WorkerStatus


class FlowBuilder:
    def __init__(self, logger):
        self.logger = logger

    def build_flow_context(self, job_id, workflow_configs):
        self.logger.info(f"## start build_flow_context, job_id: {job_id}")
        flow_context = {}
        index_type_map = {}
        for workflow_config in workflow_configs:
            workflow_type = workflow_config['type']
            workflow_index = workflow_config['index']
            workflow_args = workflow_config['args']
            index_type_map[workflow_index] = workflow_type

        for workflow_config in workflow_configs:
            workflow_type = workflow_config['type']
            workflow_index = workflow_config['index']
            workflow_args = workflow_config['args']
            worker_id = flow_utils.cat_worker_id(job_id, workflow_index, workflow_type)
            upstreams = []
            inputs_statement = []
            inputs_statement_tuple = []
            if 'upstreams' in workflow_config:
                for upstream_config in workflow_config["upstreams"]:
                    index = upstream_config['index']
                    upstream_id = flow_utils.cat_worker_id(job_id, index, index_type_map[index])
                    upstreams.append(upstream_id),
                    if 'output_input_map' in upstream_config:
                        for mapping in upstream_config.get("output_input_map", []):
                            output_index, input_index = mapping.split(":")
                            inputs_statement_tuple.append((upstream_id, int(output_index), int(input_index)))

            inputs_statement_tuple.sort(key=lambda x: x[2])
            for upstream_id, output_index, _ in inputs_statement_tuple:
                inputs_statement.append(
                    {
                        'output_index': output_index,
                        'upstream': upstream_id
                    }
                )
                
            worker_context = {
                'job_id': job_id,
                'worker_id': worker_id,
                'type': workflow_type,
                'status': WorkerStatus.PENDING,
                'args': workflow_args,
                'upstreams': upstreams,
                'inputs_statement': inputs_statement
                }
            
            self.logger.debug(f"## mid build_flow_context, work_context:\n{worker_context}")
            
            flow_context[worker_id] = worker_context
        self.logger.info(f"## end build_flow_context, flow_context:\n{flow_context}")
        
        return flow_context

    def save_flow_context(self, job_id, flow_context):
        self.logger.info(f"## start save flow context, job_id: {job_id}")
        with components.create_sql_session() as session:
            for worker_id in flow_context:
                worker_context = flow_context[worker_id]
                insert_success = job_worker_mapper.insert_job_worker(session, worker_context)
                if insert_success:
                    # insert
                    self.logger.info(f"## Save worker context successfully, job_id: {job_id}, worker_id: {worker_id}, work_context:\n{worker_context}")
                    continue
                # worker already exist
                worker_record = job_worker_mapper.query_job_worker(session,job_id, worker_id)
                worker_context['status'] = worker_record.status
                worker_context['args'] = json.loads(worker_record.args)
                worker_context['upstreams'] = json.loads(worker_record.upstreams)
                worker_context['inputs_statement'] = json.loads(worker_record.inputs_statement)
                self.logger.info(f"Load worker context successfully, job_id: {job_id}, worker_id: {worker_id}, work_context:\n{worker_context}")
            self.logger.info(f"## end save flow context, job_id: {job_id}")