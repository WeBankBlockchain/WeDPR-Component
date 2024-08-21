#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
from common import utilities
from config.binary_generator import BinaryGenerator
from config.cert_generator import CertGenerator
from config.tars_config_generator import TarsConfigGenerator


class PPCGatewayConfigGenerator:
    """
    the ppc-gateway-config-generator
    """

    def __init__(self, config, output_dir):
        self.config = config
        self.output_dir = output_dir
        self.with_tars = False
        self.binary_name = utilities.ConfigInfo.ppc_gateway_binary_name
        self.service_type = utilities.ServiceInfo.gateway_service_type

    def generate_gateway_config(self):
        utilities.print_badge("* generate gateway config, app: %s" %
                              self.config.tars_config.app_name)
        # generate the ca
        ret = CertGenerator.generate_ca_cert(
            self.config.gateway_sm_ssl, self.__generate_ca_cert_path__())
        if ret is False:
            utilities.log_error(
                "* generate ca-cert config for %s failed" % self.config.tars_config.app_name)
            return False
        for agency_config in self.config.agency_list.values():
            gateway_config = agency_config.gateway_config
            ret = self.__generate_single_gateway_config__(gateway_config)
            if ret is False:
                return False
        utilities.print_badge(
            "* generate gateway config success, app: %s" % self.config.tars_config.app_name)
        return True

    def __generate_single_gateway_config__(self, gateway_config):
        # load the config from tpl_config_path
        utilities.log_info("* generate config for ppc-gateway %s.%s" %
                           (gateway_config.app_name, gateway_config.service_name))
        # store the
        # store the config.ini
        for ip_str in gateway_config.deploy_ip:
            ip_array = ip_str.split(":")
            ip = ip_array[0]
            node_count = 1
            if len(ip_array) >= 2:
                node_count = int(ip_array[1])
            for node_index in range(node_count):
                node_name = "node" + str(node_index)
                utilities.print_badge("* generate config for ppc-gateway %s.%s.%s" %
                                      (gateway_config.app_name, gateway_config.service_name, node_name))
                config_content = utilities.load_config(
                    utilities.ConfigInfo.gateway_config_tpl_path)
                # load the common config
                self.__generate_common_config__(gateway_config, config_content)
                # load the gateway config
                listen_port = gateway_config.listen_port + node_index
                self.__generate_gateway_config_content__(
                    gateway_config, config_content, listen_port)
                # load the cache config
                self.__generate_cache_config__(gateway_config, config_content)
                # generate the shell scripts for the given ip
                ret = TarsConfigGenerator.generate_ip_shell_scripts(
                    self.__generate_ip_shell_scripts_output_path__(ip), "start_all.sh", "stop_all.sh")
                if ret is False:
                    return False
                # generate the start.sh/stop.sh for given service
                ret = TarsConfigGenerator.generate_ip_shell_scripts(
                    self.__generate_ip_shell_scripts_for_given_service__(ip, gateway_config.service_name), "start.sh",
                    "stop.sh")
                if ret is False:
                    return False
                # generate the binary
                binary_path = os.path.join(
                    self.config.tars_config.binary_path, self.binary_name)
                dst_binary_path = os.path.join(
                    self.__generate_ip_shell_scripts_for_given_service__(ip, gateway_config.service_name),
                    self.binary_name)
                ret = BinaryGenerator.generate_binary(
                    binary_path, dst_binary_path)
                if ret is False:
                    return False
                # generate the shell scripts for the node
                service_name = gateway_config.service_name
                if self.with_tars is False:
                    service_name = utilities.ConfigInfo.ppc_gateway_binary_name
                ret = TarsConfigGenerator.generate_node_shell_scripts(
                    self.__generate_node_path__(gateway_config, ip, node_name), service_name)
                if ret is False:
                    return False
                # generate the ini config
                config_output_path = self.__generate_conf_output_path__(
                    gateway_config, ip, node_name)
                ini_config_output_path = os.path.join(
                    config_output_path, utilities.ConfigInfo.config_ini_file)
                ret = utilities.store_config(
                    config_content, "ini", ini_config_output_path, gateway_config.service_name)
                if ret is False:
                    utilities.log_error("* generate config for ppc-gateway %s.%s, ip: %s failed" %
                                        (gateway_config.app_name, gateway_config.service_name, ip))
                    return False
                # generate tars config
                tars_config_path = os.path.join(
                    config_output_path, utilities.ConfigInfo.tars_config_file)
                tars_listen_port = gateway_config.tars_listen_port + node_index
                ret = TarsConfigGenerator.generate_and_store_tars_conf(utilities.ConfigInfo.tars_config_tpl_path,
                                                                       tars_config_path,
                                                                       gateway_config.app_name,
                                                                       gateway_config.service_name,
                                                                       gateway_config.servant_object_list, ip,
                                                                       tars_listen_port)
                if ret is False:
                    return False
                # generate the node config
                ret = CertGenerator.generate_node_cert(self.config.gateway_sm_ssl, self.__generate_ca_cert_path__(
                ), self.__generate_conf_output_path__(gateway_config, ip, node_name))
                if ret is False:
                    utilities.log_error(
                        "* generate config for ppc-gateway %s.%s failed for generate the node config failed" %
                        (gateway_config.app_name, gateway_config.service_name))
                    return False
                utilities.print_badge("* generate config for ppc-gateway %s.%s.%s success" %
                                      (gateway_config.app_name, gateway_config.service_name, node_name))
        utilities.log_info("* generate config for ppc-gateway %s.%s success" %
                           (gateway_config.app_name, gateway_config.service_name))
        return True

    def __generate_ca_cert_path__(self):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, "ca", self.service_type)

    def __generate_node_path__(self, config, ip, node_name):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, ip, self.service_type,
                            config.service_name, node_name)

    def __generate_conf_output_path__(self, config, ip, node_name):
        node_path = self.__generate_node_path__(config, ip, node_name)
        return os.path.join(node_path, "conf")

    def __generate_ip_shell_scripts_output_path__(self, ip):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, ip, self.service_type)

    def __generate_ip_shell_scripts_for_given_service__(self, ip, service_name):
        ip_path = self.__generate_ip_shell_scripts_output_path__(ip)
        return os.path.join(ip_path, service_name)

    def __generate_common_config__(self, config, config_content):
        """
        generate the common config
        """
        section = "agency"
        # the agency
        config_content[section]["id"] = config.agency_name
        config_content["gateway"]["holding_msg_minutes"] = str(
            config.holding_msg_minutes)
        # the peers info
        for peer in config.peers:
            key = "agency." + peer.agency
            config_content[section][key] = str(','.join(peer.endpoints))

    def __generate_gateway_config_content__(self, config, config_content, listen_port):
        """
        generate the gateway config
        """
        section = "gateway"
        # the listen_ip
        config_content[section]["listen_ip"] = config.listen_ip
        # the listen port
        config_content[section]["listen_port"] = str(listen_port)
        # the thread count
        config_content[section]["thread_count"] = str(config.thread_count)
        # sm ssl
        config_content[section]["sm_ssl"] = utilities.convert_bool_to_str(
            self.config.gateway_sm_ssl)
        # disable ssl
        config_content[section]["disable_ssl"] = utilities.convert_bool_to_str(
            self.config.gateway_disable_ssl)

    def __generate_cache_config__(self, config, config_content):
        """
        generate the cache config
        """
        section = "cache"
        config_content[section]["type"] = config.cache_type
        config_content[section]["proxy"] = str(config.cache_proxy)
        config_content[section]["obServer"] = str(config.cache_obServer)
        config_content[section]["cluster"] = str(config.cache_cluster)
        # the host
        config_content[section]["host"] = config.cache_host
        # the port
        config_content[section]["port"] = str(config.cache_port)
        # the password
        config_content[section]["password"] = str(config.cache_password)
        # the database
        config_content[section]["database"] = str(config.cache_database)
        # the pool_size
        config_content[section]["pool_size"] = str(config.cache_pool_size)
        # the connection_timeout
        config_content[section]["connection_timeout"] = str(
            config.cache_connection_timeout)
        # the socket_timeout
        config_content[section]["socket_timeout"] = str(
            config.cache_socket_timeout)
