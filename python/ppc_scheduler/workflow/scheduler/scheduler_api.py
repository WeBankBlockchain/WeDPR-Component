from abc import ABC, abstractmethod

from ppc_scheduler.workflow.common.job_context import JobContext

class SchedulerApi(ABC):
    @abstractmethod
    def run(self, job_context: JobContext, flow_context: dict):
        pass