# -*- coding: utf-8 -*-

import os

from ppc_common.ppc_utils import utils, path


def job_start_log_info(job_id):
    return f"=====================start_job_{job_id}====================="

def job_end_log_info(job_id):
    return f"======================end_job_{job_id}======================"

def worker_start_log_info(work_id):
    return f"=====================start_work_{work_id}====================="

def worker_end_log_info(work_id):
    return f"======================end_work_{work_id}======================"

def upload_job_log(storage_client, job_id, extra=None):
    job_log_path = None
    try:
        file_path = path.get_path()
        job_log_path = utils.get_log_temp_file_path(file_path, job_id)
        origin_log_path = utils.get_log_file_path(file_path)
        filter_job_log(job_id, origin_log_path, job_log_path)
        if extra is not None:
            job_log = open(job_log_path, 'a+')
            job_log.write('\n' * 3)
            job_log.write(extra)
            job_log.close()
        storage_client.upload_file(job_log_path, job_id + os.sep + utils.LOG_NAME)
    except (Exception) as e:
        print(e)
    finally:
        if os.path.exists(job_log_path):
            os.remove(job_log_path)


def read_line_inverse(file):
    file.seek(0, 2)
    current_position = file.tell()
    position = 0
    while position + current_position >= 0:
        while True:
            position -= 1
            try:
                file.seek(position, 2)
                if file.read(1) == b'\n':
                    break
            except:
                # point at file header
                file.seek(0, 0)
                break
        line = file.readline()
        yield line


def filter_job_log(job_id, origin_log_path, job_log_path):
    origin_log_file = open(origin_log_path, 'rb')
    line_list = []
    need_record = False

    # search job log
    for line in read_line_inverse(origin_log_file):
        if need_record:
            line_list.append(line)
        if not need_record and str(line).__contains__(job_end_log_info(job_id)):
            need_record = True
            line_list.append(line)
        elif str(line).__contains__(job_start_log_info(job_id)):
            break
    origin_log_file.close()

    # save log lines into temp file
    job_log_file = open(job_log_path, 'wb+')
    job_log_file.writelines(list(reversed(line_list)))
    job_log_file.close()
