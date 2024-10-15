from ppc_scheduler.node.computing_node_client.model_node_client import ModelClient
from ppc_scheduler.workflow.worker.engine.model_engine import ModelWorkerEngine
from ppc_scheduler.workflow.worker.worker import Worker


class ModelWorker(Worker):

    def __init__(self, components, job_context, worker_id, worker_type, worker_args, *args, **kwargs):
        super().__init__(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)

    def engine_run(self, worker_inputs):
        model_client_node = self.node_manager.get_node(self.worker_type)
        model_client = ModelClient(self.components.logger(), model_client_node[0], model_client_node[1])
        model_engine = ModelWorkerEngine(model_client, self.worker_type, self.worker_id, self.components, self.job_context)
        try:
            outputs = model_engine.run(*self.worker_args)
            return outputs
        finally:
            self.node_manager.release_node(model_client_node, self.worker_type)
