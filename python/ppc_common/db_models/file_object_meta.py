from ppc_common.db_models import db
from sqlalchemy import text


class FileObjectMeta(db.Model):
    __tablename__ = 't_file_object'
    file_path = db.Column(db.String(255), primary_key=True)
    file_count = db.Column(db.Integer)
    create_time = db.Column(db.TIMESTAMP(
        True), nullable=False, server_default=text('NOW()'))
    last_update_time = db.Column(db.TIMESTAMP(True), nullable=False, server_default=text(
        'CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP'))
