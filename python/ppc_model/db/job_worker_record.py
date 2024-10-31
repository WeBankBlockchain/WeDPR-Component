# -*- coding: utf-8 -*-
from datetime import datetime
from ppc_common.db_models import db


class JobWorkerRecord(db.Model):
    """
    CREATE TABLE if not exists wedpr_job_worker_table (
    worker_id VARCHAR(100),
    job_id VARCHAR(255),
    type VARCHAR(255),
    status VARCHAR(255),
    args LONGTEXT,
    upstreams TEXT,
    inputs_statement TEXT,
    outputs TEXT,
    create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (worker_id),
    INDEX job_id_idx (job_id)
    )ENGINE='InnoDB' DEFAULT CHARSET='utf8mb4' COLLATE='utf8mb4_bin' ROW_FORMAT=DYNAMIC;
    """
    __tablename__ = 'wedpr_job_worker_table'
    worker_id = db.Column(db.String(100), primary_key=True)
    job_id = db.Column(db.String(255), index=True)
    type = db.Column(db.String(255))
    status = db.Column(db.String(255))
    upstreams = db.Column(db.Text)
    inputs_statement = db.Column(db.Text)
    args = db.Column(db.Text)
    outputs = db.Column(db.Text)
    exec_result = db.Column(db.Text)
    create_time = db.Column(db.DateTime, default=datetime.now)
    update_time = db.Column(db.DateTime, onupdate=datetime.now)
