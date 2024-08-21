#!/usr/bin/python
# -*- coding: UTF-8 -*-
from common import utilities
import os


class TarsConfigGenerator:
    """
    generate the tars-config
    """
    def __generate_adapter_info__(app_name, server_name, servant_object, tars_listen_ip, tars_listen_port):
        adapter_name = "%s.%s.%sAdapter" % (
            app_name, server_name, servant_object)
        servant_name = "%s.%s.%s" % (app_name, server_name, servant_object)
        result = "<%s>\n" % adapter_name
        result = "%s\t\tallow\n" % result
        result = "%s\t\tendpoint=tcp -h %s -p %d -t 60000\n" % (
            result, tars_listen_ip, tars_listen_port)
        result = "%s\t\tmaxconns=100000\n\t\tprotocol=tars\n\t\tqueuecap=50000\n\t\tqueuetimeout=20000\n" % result
        result = "%s\t\tservant=%s\n" % (result, servant_name)
        result = "%s\t\tthreads=8\n\t\t</%s>\n" % (result, adapter_name)
        return result

    def generate_ip_shell_scripts(script_output_dir, start_shell_script_name, stop_shell_script_name):
        tars_start_all_path = os.path.join(
            script_output_dir, start_shell_script_name)
        utilities.mkdir(script_output_dir)
        if os.path.exists(tars_start_all_path) is False:
            utilities.log_debug(
                "* generate shell script, dst: %s" % tars_start_all_path)
            # tars_start_all.sh
            command = "cp %s %s" % (
                utilities.ConfigInfo.tars_start_all_tpl_path, tars_start_all_path)
            (result, output) = utilities.execute_command_and_getoutput(command)
            if result is False:
                utilities.log_error(
                    "* generate %s failed, error: %s" % (tars_start_all_path, output))
                return False
        tars_stop_all_path = os.path.join(
            script_output_dir, stop_shell_script_name)
        if os.path.exists(tars_stop_all_path) is False:
            # tars stop_all.sh
            command = "cp %s %s" % (
                utilities.ConfigInfo.tars_stop_all_tpl_path, tars_stop_all_path)
            (result, output) = utilities.execute_command_and_getoutput(command)
            if result is False:
                utilities.log_error(
                    "* generate %s failed, error: %s" % (tars_stop_all_path, output))
                return False
        utilities.log_debug(
            "* generate_ip_shell_scripts success, output: %s" % script_output_dir)
        return True

    def __update_service_name__(file_path, output_path, service_name):
        command = "cp %s %s" % (file_path, output_path)
        (ret, output) = utilities.execute_command_and_getoutput(command)
        if ret is False:
            return False
        updated_config = ""
        with open(output_path, 'r', encoding='UTF-8') as file:
            updated_config = file.read()
            updated_config = updated_config.replace(
                "@SERVICE_NAME@", "../" + service_name)
        with open(output_path, 'w', encoding='UTF-8') as file:
            file.write(updated_config)
        return True

    def generate_node_shell_scripts(script_output_dir, service_name):
        utilities.log_debug("* generate shell scripts for %s, dst: %s" %
                            (service_name, script_output_dir))
        utilities.mkdir(script_output_dir)
        # the start.sh
        output_path = os.path.join(script_output_dir, "start.sh")
        ret = TarsConfigGenerator.__update_service_name__(
            utilities.ConfigInfo.tars_start_tpl_path, output_path, service_name)
        if ret is False:
            utilities.log_error(
                "generate_node_shell_scripts %s error" % output_path)
            return False
        # the stop.sh
        output_path = os.path.join(script_output_dir, "stop.sh")
        ret = TarsConfigGenerator.__update_service_name__(
            utilities.ConfigInfo.tars_stop_tpl_path, output_path, service_name)
        if ret is False:
            utilities.log_error(
                "generate_node_shell_scripts %s error" % output_path)
            return False
        return True

    def generate_and_store_tars_conf(conf_tpl_path, output_path, app_name, server_name, servant_object_list, tars_listen_ip, tars_listen_port):
        utilities.log_debug("generate_and_store_tars_conf: %s" % conf_tpl_path)
        utilities.mkfiledir(output_path)
        # copy tpl to output_path
        if os.path.exists(output_path):
            utilities.log_error("the tars file %s already exists!")
            return False
        command = "cp %s %s" % (conf_tpl_path, output_path)
        (ret, output) = utilities.execute_command_and_getoutput(command)
        if ret is False:
            utilities.log_error("copy tpl tars config %s to %s error: %s" % (
                conf_tpl_path, output_path, output))
            return False
        # config
        module_name = "%s.%s" % (app_name, server_name)

        tars_adapters = ""
        for servant_obj in servant_object_list:
            adapter_info = TarsConfigGenerator.__generate_adapter_info__(
                app_name, server_name, servant_obj, tars_listen_ip, tars_listen_port)
            tars_adapters = "%s%s" % (tars_adapters, adapter_info)
        utilities.log_debug("tars_adapters: %s" % tars_adapters)
        updated_config = ""
        with open(output_path, 'r', encoding='UTF-8') as file:
            updated_config = file.read()
            updated_config = updated_config.replace("@TARS_APP@", app_name)
            updated_config = updated_config.replace(
                "@TARS_SERVER@", server_name)
            updated_config = updated_config.replace(
                "@MODULE_NAME@", module_name)
            updated_config = updated_config.replace(
                "@TARS_ADAPTERS@", tars_adapters)
        with open(output_path, 'w', encoding='UTF-8') as file:
            file.write(updated_config)
