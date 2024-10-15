# Note: here can't be refactored by autopep

import os
import sys
import argparse

current_file_path = os.path.abspath(__file__)
current_file_real_path = os.path.realpath(current_file_path)


current_dir = os.path.dirname(current_file_real_path)
parent_dir = os.path.dirname(current_dir)

sys.path.append(current_dir)
sys.path.append(parent_dir)
print(sys.path)

from ppc_scheduler.endpoints.restx import api
from ppc_scheduler.endpoints.job_controller import ns as job_namespace
from ppc_common.ppc_async_executor.thread_event_manager import ThreadEventManager
from ppc_scheduler.job.job_manager import JobManager
from ppc_scheduler.workflow.scheduler.scheduler import Scheduler
from ppc_scheduler.common.global_job_manager import update_job_manager
from ppc_scheduler.common.global_context import components
from ppc_scheduler.workflow.scheduler.scheduler_api import SchedulerApi
from paste.translogger import TransLogger
from flask import Flask, Blueprint
from cheroot.wsgi import Server as WSGIServer

app = Flask(__name__)

def init_thread_event_manager():
    thread_event_manager = ThreadEventManager()
    return thread_event_manager

def init_scheduler(config_data, workspace: str, logger):
    scheduler_api = Scheduler(workspace, logger=logger)
    return scheduler_api

def init_job_manager(config_data, workspace: str, thread_event_manager: ThreadEventManager, scheduler: SchedulerApi, logger):
        
    job_timeout_h = config_data['JOB_TIMEOUT_H']
    
    job_manager = JobManager(
        logger=logger,
        scheduler=scheduler,
        thread_event_manager=thread_event_manager,
        workspace=workspace,
        job_timeout_h=job_timeout_h
    )
    
    logger.info("Initialize job manager, job_timeout_h: %s", job_timeout_h)
    
    return job_manager

def initialize_app(app, config_path, log_config_path):    
    # init log first
    components.init_log(log_config_path=log_config_path)
    logger = components.logger()
    
    # init config
    config_data = components.init_config(config_path=config_path)
    # workspaces
    workspace = config_data['WORKSPACE']
    logger.info(f" ==> Initialize workspace: {workspace}")
    
    # event manager
    thread_event_manager = init_thread_event_manager()
    
    # scheduler
    scheduler = init_scheduler(config_data=config_data, workspace=workspace, logger=logger)
    
    # job manager
    job_manager = init_job_manager(config_data=config_data, workspace=workspace, thread_event_manager=thread_event_manager, scheduler=scheduler, logger=logger)
    
    update_job_manager(job_manager)
    
    # initialize application components
    components.init_all(config_data=config_data)
    components.update_thread_event_manager(thread_event_manager)

    app.config.update(config_data)
    
    blueprint = Blueprint('api', __name__, url_prefix='/api')
    api.init_app(blueprint)
    api.add_namespace(job_namespace)
    app.register_blueprint(blueprint)
    
    components.logger().info(app.url_map)

def main(config_path, log_config_path):
    
    print(f"Using config: {config_path}")
    print(f"Using logging config: {log_config_path}")
    
    initialize_app(app, config_path, log_config_path)

    app.config['SECRET_KEY'] = os.urandom(24)
    
    listen_ip = app.config['HTTP_HOST']
    listen_port = app.config['HTTP_PORT']
    thread_num = app.config['HTTP_THREAD_NUM']
    
    server = WSGIServer((listen_ip, listen_port),
                        TransLogger(app, setup_console_handler=False), numthreads=thread_num)

    protocol = 'http'
    message = f"Starting wedpr scheduler server at {protocol}://{listen_ip}:{listen_port}"
    print(message)
    components.logger().info(message)
    server.start()

if __name__ == "__main__":
    # Create ArgumentParser
    parser = argparse.ArgumentParser(description='wedpr scheduler')
    # Add argument
    parser.add_argument('--config', default='./conf/application.yml', 
                        help='Path to the configuration file')
    parser.add_argument('--log_config', default='./conf/logging.conf',
                        help='Path to the logging configuration file')
    # Parser argument
    args = parser.parse_args()

    # Run main program
    main(config_path=args.config, log_config_path=args.log_config)
