
from ppc_common.db_models import db


class ComputingNodeRecord(db.Model):
    __tablename__ = 'wedpr_computing_node'
    id = db.Column(db.String(255), primary_key=True)
    url = db.Column(db.String(255))
    type = db.Column(db.String(255))
    token = db.Column(db.String(255))
    loading = db.Column(db.Integer)
