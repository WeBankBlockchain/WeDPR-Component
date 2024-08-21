
from ppc_common.db_models import db


class JobUnitRecord(db.Model):
    __tablename__ = 't_job_unit'
    unit_id = db.Column(db.String(100), primary_key=True)
    job_id = db.Column(db.String(255), index=True)
    type = db.Column(db.String(255))
    status = db.Column(db.String(255), index=True)
    upstream_units = db.Column(db.Text)
    inputs_statement = db.Column(db.Text)
    outputs = db.Column(db.Text)
    update_time = db.Column(db.BigInteger)
