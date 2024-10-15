import os
from ppc_scheduler.workflow.worker.engine.work_engine import WorkerEngine
from ppc_scheduler.common.global_context import components

class ShellEngine(WorkerEngine):
    
    def __init__(self, cmd: str) -> None:
        super().__init__()
        self.cmd = cmd
    
    def run(self, *args):
        # print("shell engine is processing.")
        components.logger().info(f"shell engine is processing, cmd: {self.cmd}")
        
        result = os.system(self.cmd)
        return list(str(result))