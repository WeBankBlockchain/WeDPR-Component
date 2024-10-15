import json

def deserialize_worker_outputs(outputs_str):
    return json.loads(outputs_str)

def serialize_worker_outputs(outputs):
    return json.dumps(outputs)

def deserialize_upstreams(upstreams_str):
    return json.loads(upstreams_str)

def serialize_upstreams(upstreams):
    return json.dumps(upstreams)

def deserialize_inputs_statement(inputs_statement_str):
    return json.loads(inputs_statement_str)

def serialize_inputs_statement(inputs_statement):
    return json.dumps(inputs_statement)