from ppc_scheduler.database import computing_node_mapper
from ppc_scheduler.workflow.common.worker_type import WorkerType


class ComputingNodeManager:
    type_map = {
        WorkerType.T_PSI: 'PSI',
        WorkerType.T_MPC: 'MPC',
        WorkerType.T_MODEL: 'MODEL'
    }

    def __init__(self, components):
        self.components = components

    def add_node(self, node_id: str, url: str, worker_type: str):
        with self.components.create_sql_session() as session:
            computing_node_mapper.insert_computing_node(
                session, node_id, url, self.type_map[worker_type], 0)

    def remove_node(self, url: str, worker_type: str):
        with self.components.create_sql_session() as session:
            computing_node_mapper.delete_computing_node(
                session, url, self.type_map[worker_type])

    def get_node(self, worker_type: str):
        with self.components.create_sql_session() as session:
            return computing_node_mapper.get_and_update_min_loading_url(session, self.type_map[worker_type])

    def release_node(self, url: str, worker_type: str):
        with self.components.create_sql_session() as session:
            return computing_node_mapper.release_loading(session, url, self.type_map[worker_type])
