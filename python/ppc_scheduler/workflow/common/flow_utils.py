from ppc_scheduler.workflow.common.worker_type import WorkerType


def cat_worker_id(job_id, index, worker_type):
    return f"{job_id}_{index}_{worker_type}"


def success_id(job_id):
    return cat_worker_id(job_id, 0, WorkerType.T_ON_SUCCESS)


def failure_id(job_id):
    return cat_worker_id(job_id, 0, WorkerType.T_ON_FAILURE)


def to_origin_inputs(worker_inputs):
    inputs = []
    for each in worker_inputs:
        output_index = each['output_index']
        upstream_outputs = each['upstream_outputs']
        inputs.append(upstream_outputs[output_index])
    return inputs


def to_worker_inputs(job_workers, inputs_statement):
    worker_inputs = []
    for each in inputs_statement:
        output_index = each['output_index']
        upstream = each['upstream']
        worker_inputs.append({
            'output_index': output_index,
            'upstream_outputs': job_workers[upstream]
        })
    return worker_inputs
