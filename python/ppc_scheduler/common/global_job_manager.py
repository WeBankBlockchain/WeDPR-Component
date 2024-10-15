

from ppc_scheduler.job.job_manager import JobManager


global_job_manager: JobManager = None

def update_job_manager(job_manager):
    global global_job_manager
    global_job_manager = job_manager
    
def get_job_manager():
    global global_job_manager
    return global_job_manager

