from ppc_scheduler.workflow.worker.worker import Worker
from ppc_scheduler.workflow.worker.engine.shell_engine import ShellEngine


class ShellWorker(Worker):

    def __init__(self, components, job_context, worker_id, worker_type, worker_args, *args, **kwargs):
        super().__init__(components, job_context, worker_id, worker_type, worker_args, *args, **kwargs)

    def engine_run(self, worker_inputs):   
        
        # self.log_worker()
             
        mpc_engine = ShellEngine(cmd=self.worker_args[0])
        outputs = mpc_engine.run()
        return outputs
