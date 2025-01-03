
from ppc_model.common.base_context import BaseContext
from ppc_model.common.initializer import Initializer
from ppc_model.common.protocol import TaskRole


class Context(BaseContext):

    def __init__(self, job_id: str, task_id: str, user: str, components: Initializer, role: TaskRole = None):
        super().__init__(job_id, components.config_data['JOB_TEMP_DIR'], user)
        self.my_agency_id = components.config_data['AGENCY_ID']
        self.task_id = task_id
        self.components = components
        self.role = role
