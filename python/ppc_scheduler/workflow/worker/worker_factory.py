from ppc_common.ppc_utils.exception import PpcErrorCode, PpcException
from ppc_scheduler.common.global_context import components
from ppc_scheduler.workflow.common.worker_type import WorkerType
from ppc_scheduler.workflow.worker.api_worker import ApiWorker
from ppc_scheduler.workflow.worker.default_worker import DefaultWorker
from ppc_scheduler.workflow.worker.model_worker import ModelWorker
from ppc_scheduler.workflow.worker.mpc_worker import MpcWorker
from ppc_scheduler.workflow.worker.psi_worker import PsiWorker
from ppc_scheduler.workflow.worker.python_worker import PythonWorker
from ppc_scheduler.workflow.worker.shell_worker import ShellWorker


class WorkerFactory:

    @staticmethod
    def build_worker(job_context, worker_id, worker_type, worker_args, *args, **kwargs):
        if worker_type == WorkerType.T_API:
            return ApiWorker(components, job_context, worker_id, worker_type, worker_args, *args, *kwargs)
        elif worker_type == WorkerType.T_PYTHON:
            return PythonWorker(components, job_context, worker_id, worker_type, worker_args, *args, *kwargs)
        elif worker_type == WorkerType.T_SHELL:
            return ShellWorker(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)
        elif worker_type == WorkerType.T_PSI:
            return PsiWorker(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)
        elif worker_type == WorkerType.T_MPC:
            return MpcWorker(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)
        elif worker_type == WorkerType.T_MODEL:
            return ModelWorker(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)
        elif worker_type == WorkerType.T_ON_SUCCESS or \
                worker_type == WorkerType.T_ON_FAILURE:
            return DefaultWorker(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)
        else:
            raise PpcException(PpcErrorCode.UNSUPPORTED_WORK_TYPE,
                               f"Unsupported worker type: {worker_type}")
