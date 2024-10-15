
from datetime import datetime
from ppc_common.db_models import db


class JobWorkerRecord(db.Model):
    __tablename__ = 'wedpr_scheduler_job_worker_table'
    worker_id = db.Column(db.String(127), primary_key=True)
    job_id = db.Column(db.String(255), index=True)
    type = db.Column(db.String(255))
    status = db.Column(db.String(255))
    upstreams = db.Column(db.Text)
    inputs_statement = db.Column(db.Text)
    args = db.Column(db.Text)
    outputs = db.Column(db.Text)
    create_time = db.Column(db.DateTime, default=datetime.now)
    update_time = db.Column(db.DateTime, onupdate=datetime.now)
