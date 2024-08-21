from ppc_common.db_models import db


class FilePathRecord(db.Model):
    __tablename__ = 't_file_path'
    path = db.Column(db.String(255), primary_key=True)
    storage_type = db.Column(db.String(255))
    file_id = db.Column(db.String(255))
    file_hash = db.Column(db.String(255))
    create_time = db.Column(db.BigInteger)
