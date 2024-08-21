# -*- coding: utf-8 -*-
import os
import yaml


dirName, _ = os.path.split(os.path.abspath(__file__))
config_path = '{}/application.yml'.format(dirName)

CONFIG_DATA = {}


def read_config():
    global CONFIG_DATA
    with open(config_path, 'rb') as f:
        CONFIG_DATA = yaml.safe_load(f.read())


read_config()
