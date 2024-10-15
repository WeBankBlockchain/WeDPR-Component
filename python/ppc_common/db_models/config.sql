
CREATE TABLE wedpr_scheduler_job_worker_table (
    worker_id VARCHAR(100),
    job_id VARCHAR(255),
    type VARCHAR(255),
    status VARCHAR(255),
    args TEXT,
    upstreams TEXT,
    inputs_statement TEXT,
    outputs TEXT,
    create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (worker_id),
    INDEX job_id_idx (job_id)
)ENGINE='InnoDB' DEFAULT CHARSET='utf8mb4' COLLATE='utf8mb4_bin' ROW_FORMAT=DYNAMIC;

CREATE TABLE wedpr_scheduler_job_table (
    job_id VARCHAR(255),
    request TEXT,
    status TEXT,
    create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (job_id)
)ENGINE='InnoDB' DEFAULT CHARSET='utf8mb4' COLLATE='utf8mb4_bin' ROW_FORMAT=DYNAMIC;

CREATE TABLE wedpr_computing_node (
    id VARCHAR(255),
    url VARCHAR(255),
    type VARCHAR(255),
    loading INT,
    token VARCHAR(255) DEFAULT '',
    PRIMARY KEY (id)
)ENGINE='InnoDB' DEFAULT CHARSET='utf8mb4' COLLATE='utf8mb4_bin' ROW_FORMAT=DYNAMIC;


INSERT INTO wedpr_computing_node (id, url, type, loading, token)
VALUES
    ("001", '127.0.0.1:10200', 'PSI', 0, ''),
    ("002", '127.0.0.1:10201', 'MPC', 0, ''),
    ("003", '127.0.0.1:10202', 'MODEL', 0, '');
