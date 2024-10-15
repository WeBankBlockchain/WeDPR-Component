from ppc_scheduler.node.computing_node_client.mpc_node_client import MpcClient
from ppc_scheduler.workflow.worker.engine.mpc_engine import MpcWorkerEngine
from ppc_scheduler.workflow.worker.worker import Worker


class MpcWorker(Worker):

    def __init__(self, components, job_context, worker_id, worker_type, worker_args, *args, **kwargs):
        super().__init__(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)

    def engine_run(self, worker_inputs) -> list:
        node_endpoint = self.node_manager.get_node(self.worker_type)
        mpc_client = MpcClient(node_endpoint)
        mpc_engine = MpcWorkerEngine(mpc_client, self.worker_type, self.components, self.job_context)
        try:
            outputs = mpc_engine.run()
            return outputs
        finally:
            self.node_manager.release_node(node_endpoint, self.worker_type)
