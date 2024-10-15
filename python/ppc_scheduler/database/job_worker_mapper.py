import datetime
import json
# from sqlalchemy.dialects.mysql import insert
from sqlalchemy import and_, update, insert
from sqlalchemy.exc import NoResultFound

from ppc_common.db_models.job_worker_record import JobWorkerRecord
from ppc_common.ppc_utils import utils
from ppc_scheduler.workflow.common import codec
from ppc_scheduler.workflow.common.worker_status import WorkerStatus

def insert_job_worker(session, worker):
    upstreams_str = json.dumps(worker['upstreams'])
    inputs_statement_str = json.dumps(worker['inputs_statement'])
    args_str=json.dumps(worker['args'])
    
    stmt = insert(JobWorkerRecord).prefix_with(" IGNORE").values(
        worker_id=worker['worker_id'],
        job_id=worker['job_id'],
        type=worker['type'],
        status=WorkerStatus.PENDING,
        args=args_str,
        upstreams=upstreams_str,
        inputs_statement=inputs_statement_str
    )
    
    result = session.execute(stmt)
    return result.rowcount > 0

def query_job_worker(session, job_id, worker_id):
    try:
        return session.query(JobWorkerRecord).filter(
            and_(JobWorkerRecord.worker_id == worker_id,
                 JobWorkerRecord.job_id == job_id)).one()
    except NoResultFound:
        return None


def update_job_worker(session, job_id, worker_id, status, outputs):
    stmt = (
        update(JobWorkerRecord).where(
            and_(
                JobWorkerRecord.job_id == job_id,
                JobWorkerRecord.worker_id == worker_id
            )
        ).values(
            status=status,
            outputs=codec.serialize_worker_outputs(outputs)
        )
    )
    result = session.execute(stmt)
    return result.rowcount > 0
