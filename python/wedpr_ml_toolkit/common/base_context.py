import os


class BaseContext:

    def __init__(self, project_id, user_name, pws_endpoint=None, hdfs_endpoint=None, token=None):
    
        self.project_id = project_id
        self.user_name = user_name
        self.pws_endpoint = pws_endpoint
        self.hdfs_endpoint = hdfs_endpoint
        self.token = token
        self.workspace = './milestone2'
