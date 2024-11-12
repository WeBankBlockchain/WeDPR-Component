# -*- coding: utf-8 -*-
from ppc_model.network.wedpr_model_transport_api import ModelTransportApi
from ppc_model.network.wedpr_model_transport_api import ModelRouterApi


class MockModelTransportApi(ModelTransportApi):
    def __init__(self, agency_name):
        self.agency_name = agency_name
        self.msg_queue = {}

    @staticmethod
    def get_topic(task_id: str, task_type: str, dst_agency: str):
        return f"{dst_agency}_{task_id}{task_type}"

    def push_by_nodeid(self, task_id: str, task_type: str, dst_node: str, payload: bytes, seq: int = 0):
        self.msg_queue.update({MockModelTransportApi.get_topic(
            task_id, task_type, self.agency_name): payload})

    def pop(self, task_id: str, task_type: str, dst_inst: str):
        topic = MockModelTransportApi.get_topic(task_id, task_type, dst_inst)
        if topic in self.msg_queue.keys():
            payload = self.msg_queue.get(topic)
            self.msg_queue.pop(topic)
            return payload
        raise Exception(f"Not receive the message of topic:"
                        f" {self.get_topic(task_id, task_type, dst_inst)} "
                        f"even after the task has been killed!")


class MockModelRouterApi(ModelRouterApi):
    def __init__(self, participant_id_list):
        self.participant_id_list = participant_id_list
        self.transports = {}
        for participant in self.participant_id_list:
            self.transports.update(
                {participant, MockModelTransportApi(participant)})

    def push(self, task_id: str, task_type: str, dst_agency: str, payload: bytes, seq: int = 0):
        transport = self.transports.get(dst_agency)
        dst_nodeid = f"{dst_agency}_node"
        transport.push_by_nodeid(task_id, task_type, dst_nodeid, payload, seq)

    def pop(self, task_id: str, task_type: str, from_inst: str) -> bytes:
        transport = self.transports.get(from_inst)
        return transport.pop(task_id, task_type, from_inst)
