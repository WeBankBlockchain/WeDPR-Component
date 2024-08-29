from ppc_scheduler.common.global_context import components
from ppc_scheduler.workflow.common.worker_type import WorkerType
from ppc_scheduler.workflow.worker.api_worker import ApiWorker
from ppc_scheduler.workflow.worker.exit_worker import ExitWorker
from ppc_scheduler.workflow.worker.model_worker import ModelWorker
from ppc_scheduler.workflow.worker.mpc_worker import MpcWorker
from ppc_scheduler.workflow.worker.psi_worker import PsiWorker
from ppc_scheduler.workflow.worker.python_worker import PythonWorker
from ppc_scheduler.workflow.worker.shell_worker import ShellWorker


class WorkerFactory:

    @staticmethod
    def build_worker(job_context, worker_id, worker_type):
        if worker_type == WorkerType.T_API:
            return ApiWorker(components, job_context, worker_id, worker_type)
        elif worker_type == WorkerType.T_PYTHON:
            return PythonWorker(components, job_context, worker_id, worker_type)
        elif worker_type == WorkerType.T_SHELL:
            return ShellWorker(components, job_context, worker_id, worker_type)
        elif worker_type == WorkerType.T_PSI:
            return PsiWorker(components, job_context, worker_id, worker_type)
        elif worker_type == WorkerType.T_MPC:
            return MpcWorker(components, job_context, worker_id, worker_type)
        elif worker_type == WorkerType.T_PREPROCESSING or \
                worker_type == WorkerType.T_FEATURE_ENGINEERING or \
                worker_type == WorkerType.T_TRAINING or \
                worker_type == WorkerType.T_PREDICTION:
            return ModelWorker(components, job_context, worker_id, worker_type)
        else:
            return ExitWorker(components, job_context, worker_id, worker_type)
