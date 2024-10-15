import time
from prefect import Flow
from prefect.executors import LocalDaskExecutor
from prefect.triggers import all_successful, any_failed

from ppc_scheduler.workflow.common import flow_utils
from ppc_scheduler.workflow.common.job_context import JobContext
from ppc_scheduler.workflow.common.worker_status import WorkerStatus
from ppc_scheduler.workflow.common.worker_type import WorkerType
from ppc_scheduler.workflow.builder.flow_builder import FlowBuilder
from ppc_scheduler.workflow.worker.worker_factory import WorkerFactory
from ppc_scheduler.workflow.scheduler.scheduler_api import SchedulerApi


class Scheduler(SchedulerApi):
    def __init__(self, workspace, logger):
        self.workspace = workspace
        self.logger = logger

    def run(self, job_context: JobContext, flow_context: dict):
 
        job_id = job_context.job_id
        job_flow = Flow(f"job_flow_{job_id}")

        finish_job_on_success = WorkerFactory.build_worker(
            job_context,
            flow_utils.success_id(job_id),
            WorkerType.T_ON_SUCCESS,
            None
            )

        finish_job_on_success.trigger = all_successful
        finish_job_on_success.bind(worker_status=WorkerStatus.PENDING, worker_inputs=[], flow=job_flow)
        job_flow.add_task(finish_job_on_success)

        # set reference task to bind job flow status
        job_flow.set_reference_tasks([finish_job_on_success])

        # create a final job worker to handle failure
        finish_job_on_failure = WorkerFactory.build_worker(
            job_context,
            flow_utils.failure_id(job_id),
            WorkerType.T_ON_FAILURE,
            None
            )

        # do finish_job_on_failure while any job worker failed
        finish_job_on_failure.trigger = any_failed
        finish_job_on_failure.bind(worker_status=WorkerStatus.PENDING, worker_inputs=[], flow=job_flow)
        job_flow.add_task(finish_job_on_failure)
        
        job_workers = {}
        # create main job workers
        for worker_id in flow_context:
            worker_type = flow_context[worker_id]['type']
            worker_args = flow_context[worker_id]['args']
            
            job_worker = WorkerFactory.build_worker(job_context=job_context, worker_id=worker_id, worker_type=worker_type, worker_args=worker_args)
            job_flow.add_task(job_worker)
            job_workers[worker_id] = job_worker

            # set upstream for final job
            finish_job_on_success.set_upstream(job_worker, flow=job_flow)
            finish_job_on_failure.set_upstream(job_worker, flow=job_flow)

        # customize main job workers
        for worker_id in flow_context:
            # set upstream
            upstreams = flow_context[worker_id]['upstreams']
            status = flow_context[worker_id]['status']
            for upstream in upstreams:
                if upstream not in job_workers:
                    raise Exception(-1, f"Not found upstream job worker : {upstream}, "
                                        f"job_id: {job_context.job_id}")
                job_workers[worker_id].set_upstream(job_workers[upstream], flow=job_flow)
                
            # bind worker inputs
            inputs_statement = flow_context[worker_id]['inputs_statement']
            worker_inputs = flow_utils.to_worker_inputs(job_workers, inputs_statement)
            job_workers[worker_id].bind(worker_status=status,
                                        worker_inputs=worker_inputs, flow=job_flow)

        # enable parallel execution
        job_flow.executor = LocalDaskExecutor()

        #
        start_time = time.time()

        # run dag workflow
        job_flow_state = job_flow.run()
        
        end_time = time.time()
        
        self.logger.info(f" ## Job worker result, job: {job_id}, success: {job_flow_state.is_successful()}, costs: {end_time - start_time}, flow_state: {job_flow_state}")

        # save workflow view as file
        job_flow.visualize(job_flow_state, job_context.workflow_view_path, 'svg')
        
        if not job_flow_state.is_successful():
            raise Exception(-1, f"Job run failed, job_id: {job_id}")
