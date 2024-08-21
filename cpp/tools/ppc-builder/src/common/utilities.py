#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import re
import os
import subprocess
import logging
import configparser

logging.basicConfig(format='%(message)s',
                    level=logging.INFO)


class ServiceInfo:
    ssl_file_list = ["ca.crt", "ssl.key", "ssl.crt"]
    sm_ssl_file_list = ["sm_ca.crt", "sm_ssl.key",
                        "sm_ssl.crt", "sm_enssl.key", "sm_enssl.crt"]

    node_service_postfix = "NodeService"
    gateway_servant = "GatewayService"
    gateway_servant_obj = ["GatewayServiceObj"]

    node_servant = ["FrontService"]
    node_servant_object = ["FrontServiceObj"]

    cert_generation_script_path = "src/scripts/gen_cert.sh"
    node_service_type = "node"
    gateway_service_type = "gateway"
    supported_service_type = [node_service_type, gateway_service_type]


class ConfigInfo:
    config_ini_file = "config.ini"
    tars_config_file = "tars.conf"

    tpl_abs_path = "src/tpl/"
    pwd_path = os.getcwd()
    node_config_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "config.ini.node")
    gateway_config_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "config.ini.gateway")
    tars_config_tpl_path = os.path.join(pwd_path, tpl_abs_path, "tars.conf")

    tars_start_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "tars_start.sh")
    tars_stop_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "tars_stop.sh")

    tars_start_all_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "tars_start_all.sh")
    tars_stop_all_tpl_path = os.path.join(
        pwd_path, tpl_abs_path, "tars_stop_all.sh")

    ppc_gateway_binary_name = "ppc-gateway-service"
    ppc_node_binary_name = "ppc-pro-node"


class CommandInfo:
    generate_config = "genconfig"
    extend_config = "extend"
    supported_command_list = [generate_config, extend_config]


def log_error(error_msg):
    logging.error("\033[31m%s \033[0m" % error_msg)


def log_info(error_msg):
    logging.info("\033[32m%s \033[0m" % error_msg)


def format_info(info):
    return ("\033[32m%s \033[0m" % info)


def log_debug(error_msg):
    logging.debug("%s" % error_msg)


def get_item_value(config, key, default_value, must_exist, desc):
    if key in config:
        return config[key]
    if must_exist:
        raise Exception("the value for %s.%s must be set" % (desc, key))
    return default_value


def get_value(config, section, key, default_value, must_exist):
    if section in config and key in config[section]:
        return config[section][key]
    if must_exist:
        raise Exception("the value for %s must be set" % key)
    return default_value


def execute_command_and_getoutput(command):
    status, output = subprocess.getstatusoutput(command)
    if status != 0:
        log_error(
            "execute command %s failed, error message: %s" % (command, output))
        return (False, output)
    return (True, output)


def execute_command(command):
    (ret, result) = execute_command_and_getoutput(command)
    return ret


def mkdir(path):
    if not os.path.exists(path):
        os.makedirs(path)


def removeDir(path):
    if os.path.exists(path):
        os.removedirs(path)


def mkfiledir(filepath):
    parent_dir = os.path.abspath(os.path.join(filepath, ".."))
    if os.path.exists(parent_dir) is False:
        mkdir(parent_dir)


def generate_service_name(prefix, service_name):
    return prefix + service_name


def convert_bool_to_str(value):
    if value is True:
        return "true"
    return "false"


def print_split_info():
    log_info("=========================================================")


def print_badge(badge):
    log_info("----------- %s -----------" % badge)


def file_must_exist(file_path):
    if not os.path.exists(file_path):
        log_error("%s does not exist, please check" % file_path)
        sys.exit(-1)


def store_config(config_content, config_type, config_path, desc):
    """
    store the generated genesis config content for given node
    """
    if os.path.exists(config_path):
        log_error("* store %s config for %s failed for the config %s already exists." %
                  (config_type, desc, config_path))
        return False
    log_info("* store %s config for %s\n\t path: %s" %
             (config_type, desc, config_path))

    if os.path.exists(os.path.dirname(config_path)) is False:
        mkdir(os.path.dirname(config_path))

    with open(config_path, 'w') as configFile:
        config_content.write(configFile)
        log_info("* store %s config for %s success" %
                 (config_type, desc))
    return True


def load_config(file_path):
    # load the config from tpl_config_path
    config_content = configparser.ConfigParser(
        comment_prefixes='/', allow_no_value=True)
    # to distinguish uppercase/lowercase letters
    config_content.optionxform = str
    config_content.read(file_path)
    return config_content


def check_service_name(tag, service_name):
    """
    Note: tars service name can only contain letters and numbers
    """
    service_name_len = len(service_name)
    ret = re.search(r'^[A-Za-z0-9]+', service_name).span()
    if ret is None or (ret[0] != 0 or ret[1] != service_name_len):
        raise Exception(
            "the %s must be letters|numbers, invalid value: %s" % (tag, service_name))
