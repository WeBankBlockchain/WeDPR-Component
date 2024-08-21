#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
from common import utilities
from config.binary_generator import BinaryGenerator
from config.cert_generator import CertGenerator
from config.tars_config_generator import TarsConfigGenerator


class PPCNodeConfigGenerator:
    """
    the ppc-node-config-generator
    """

    def __init__(self, config, output_dir):
        self.config = config
        self.output_dir = output_dir
        self.with_tars = False
        self.binary_name = utilities.ConfigInfo.ppc_node_binary_name
        self.service_type = utilities.ServiceInfo.node_service_type

    def generate_node_config(self):
        utilities.print_badge("* generate_node_config")
        # generate the ca cert for rpc
        ret = CertGenerator.generate_ca_cert(
            self.config.rpc_sm_ssl, self.__generate_ca_cert_path__())
        if ret is False:
            utilities.log_error(
                "* generate_node_config failed for generate ca error, app: %s" % (self.config.tars_config.app_name))
            return False
        for agency_config in self.config.agency_list.values():
            for node_config in agency_config.node_list.values():
                for ip_str in node_config.deploy_ip:
                    ip_array = ip_str.split(":")
                    ip = ip_array[0]
                    node_count = 1
                    if len(ip_array) >= 2:
                        node_count = int(ip_array[1])
                    for node_index in range(node_count):
                        node_name = "node" + str(node_index)
                        if self.__generate_single_node_config__(node_config, ip, node_name, node_index) is False:
                            return False
        utilities.print_badge("* generate_node_config success")
        return True

    def __generate_single_node_config__(self, node_config, ip, node_name, node_index):
        utilities.print_badge("* generate node config for %s.%s.%s, ip: %s" %
                              (node_config.app_name, node_config.service_name, node_name, ip))
        # generate the shell scripts for the given ip
        ret = TarsConfigGenerator.generate_ip_shell_scripts(
            self.__generate_ip_output_path__(ip), "start_all.sh", "stop_all.sh")
        if ret is False:
            return False
        # generate the start.sh/stop.sh for given service
        ret = TarsConfigGenerator.generate_ip_shell_scripts(
            self.__generate_ip_shell_scripts_for_given_service__(ip, node_config.service_name), "start.sh", "stop.sh")
        if ret is False:
            return False
        # copy the binary
        binary_path = os.path.join(
            self.config.tars_config.binary_path, self.binary_name)
        dst_binary_path = os.path.join(
            self.__generate_ip_shell_scripts_for_given_service__(ip, node_config.service_name), self.binary_name)
        ret = BinaryGenerator.generate_binary(binary_path, dst_binary_path)
        if ret is False:
            return False
        # generate the node config
        node_path = self.__generate_node_conf_path__(
            node_config, ip, node_name)
        private_key_path = self.__generate_node_conf_path__(
            node_config, ip, node_name)
        tars_output_path = private_key_path
        if self.__generate_single_node_inner_config__(utilities.ConfigInfo.node_config_tpl_path,
                                                      utilities.ConfigInfo.tars_config_tpl_path, node_path,
                                                      private_key_path, tars_output_path, node_config, ip,
                                                      node_index) is False:
            utilities.log_error("* generate node config for %s.%s, ip: %s failed" %
                                (node_config.app_name, node_config.service_name, ip))
            return False
        # generate the node shell_scipts
        service_name = node_config.service_name
        if self.with_tars is False:
            service_name = utilities.ConfigInfo.ppc_node_binary_name
            ret = TarsConfigGenerator.generate_node_shell_scripts(
                self.__generate_node_path__(node_config, ip, node_name), service_name)
            if ret is False:
                return False
        # generate the node cert(for rpc)
        ret = CertGenerator.generate_node_cert(self.config.rpc_sm_ssl, self.__generate_ca_cert_path__(
        ), self.__generate_node_conf_path__(node_config, ip, node_name))
        if ret is False:
            utilities.log_error("* generate node config for %s.%s, ip: %s failed for generate rpc cert failed" %
                                (node_config.app_name, node_config.service_name, ip))
            return False
        utilities.print_badge("* generate node config for %s.%s.%s, ip: %s success" %
                              (node_config.app_name, node_config.service_name, node_name, ip))
        return True

    def __generate_single_node_inner_config__(self, tpl_config_path, tars_config_tpl_path, node_path, private_key_path,
                                              tars_output_path, node_config, ip, node_index):
        config_content = utilities.load_config(tpl_config_path)
        utilities.log_debug(
            "__generate_single_node_config__, load config.ini from %s" % tpl_config_path)
        # generate the private key
        (ret, private_key) = CertGenerator.generate_private_key(private_key_path)
        if ret is False:
            return False
        # load the common config
        self.__generate_common_config__(
            config_content, node_config)
        # load the rpc config
        self.__generate_rpc_config__(
            config_content, node_config.rpc_config, node_index)
        # TODO: check the configuration
        # load the storage config
        self.__generate_storage_config__(
            config_content, node_config.storage_config)
        # load the hdfs_storage_config
        self.__generate_hdfs_storage_config__(
            config_content, node_config.hdfs_storage_config)
        # load the ra2018psi config
        self.__generate_ra2018psi_config__(
            config_content, node_config.ra2018psi_config)
        # load the gateway tars config
        self.__generate_tars_gateway_config__(
            config_content, node_config.gateway_config)
        # store the config
        ini_config_output_path = os.path.join(
            node_path, utilities.ConfigInfo.config_ini_file)
        ret = utilities.store_config(
            config_content, "ini", ini_config_output_path, node_config.service_name)
        if ret is False:
            return False
        # load the tars config
        self.__generate_tars_config__(
            tars_config_tpl_path, tars_output_path, node_config, ip, node_index)
        return True

    def __generate_ca_cert_path__(self):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, "ca", self.service_type)

    def __generate_node_path__(self, node_config, ip, node_name):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, ip, self.service_type,
                            node_config.service_name, node_name)

    def __generate_node_conf_path__(self, node_config, ip, node_name):
        node_path = self.__generate_node_path__(node_config, ip, node_name)
        return os.path.join(node_path, "conf")

    def __generate_ip_output_path__(self, ip):
        return os.path.join(self.output_dir, self.config.tars_config.app_name, ip, self.service_type)

    def __generate_ip_shell_scripts_for_given_service__(self, ip, service_name):
        ip_path = self.__generate_ip_output_path__(ip)
        return os.path.join(ip_path, service_name)

    def __generate_common_config__(self, config_content, node_config):
        """
        generate the common config
        """
        # the agency config
        config_content["agency"]["id"] = node_config.agency_name
        # the holding_msg_minutes
        config_content["tars_gateway"]["holding_msg_minutes"] = str(
            node_config.holding_msg_minutes)
        #  disable ra2018 or not
        config_content["agency"]["disable_ra2018"] = utilities.convert_bool_to_str(
            node_config.disable_ra2018)
        # the crypto config
        config_content["crypto"]["sm_crypto"] = utilities.convert_bool_to_str(
            self.config.sm_crypto)

    def __generate_rpc_config__(self, config_content, rpc_config, node_index):
        """
        generate the rpc config
        """
        section_name = "rpc"
        # listen_ip
        config_content[section_name]["listen_ip"] = rpc_config.listen_ip
        # listen_port
        rpc_listen_port = rpc_config.listen_port + node_index
        config_content[section_name]["listen_port"] = str(rpc_listen_port)
        # sm_ssl
        config_content[section_name]["sm_ssl"] = utilities.convert_bool_to_str(
            self.config.rpc_sm_ssl)
        # disable_ssl
        config_content[section_name]["disable_ssl"] = utilities.convert_bool_to_str(
            self.config.rpc_disable_ssl)

    def __generate_storage_config__(self, config_content, storage_config):
        """
        generate the storage config
        """
        if storage_config is None:
            utilities.log_error("Must set the mysql-storage-config!")
            sys.exit(-1)

        section_name = "storage"
        config_content[section_name]["host"] = storage_config.host
        config_content[section_name]["port"] = str(storage_config.port)
        config_content[section_name]["user"] = storage_config.user
        config_content[section_name]["password"] = storage_config.password
        config_content[section_name]["database"] = storage_config.database

    def __generate_hdfs_storage_config__(self, config_content, hdfs_storage_config):
        if hdfs_storage_config is None:
            return
        section_name = "hdfs_storage"
        config_content[section_name]["user"] = hdfs_storage_config.user
        config_content[section_name]["name_node"] = hdfs_storage_config.name_node
        config_content[section_name]["name_node_port"] = str(
            hdfs_storage_config.name_node_port)
        config_content[section_name]["token"] = hdfs_storage_config.token

    def __generate_ra2018psi_config__(self, config_content, ra2018psi_config):
        """
        generate the ra2018psi config
        """
        section_name = "ra2018psi"
        config_content[section_name]["database"] = ra2018psi_config.database
        config_content[section_name]["cuckoofilter_capacity"] = str(
            ra2018psi_config.cuckoofilter_capacity)
        config_content[section_name]["cuckoofilter_tagBits"] = str(
            ra2018psi_config.cuckoofilter_tagBits)
        config_content[section_name]["cuckoofilter_buckets_num"] = str(
            ra2018psi_config.cuckoofilter_buckets_num)
        config_content[section_name]["cuckoofilter_max_kick_out_count"] = str(
            ra2018psi_config.cuckoofilter_max_kick_out_count)
        config_content[section_name]["trash_bucket_size"] = str(
            ra2018psi_config.trash_bucket_size)
        config_content[section_name]["cuckoofilter_cache_size"] = str(
            ra2018psi_config.cuckoofilter_cache_size)
        config_content[section_name]["psi_cache_size"] = str(
            ra2018psi_config.psi_cache_size)
        config_content[section_name]["data_batch_size"] = str(
            ra2018psi_config.data_batch_size)
        config_content[section_name]["use_hdfs"] = utilities.convert_bool_to_str(
            ra2018psi_config.use_hdfs)

    def __generate_tars_gateway_config__(self, config_content, tars_gateway_config):
        section_name = "tars_gateway"
        config_content[section_name]["name"] = tars_gateway_config.service_name
        i = 0
        for endpoint in tars_gateway_config.endpoints:
            key = "proxy.%d" % i
            config_content[section_name][key] = endpoint
            i = i + 1

    def __generate_tars_config__(self, tars_config_tpl_path, tars_config_dir, node_config, ip, node_index):
        """
        generate the tars config
        """
        tars_config_path = os.path.join(
            tars_config_dir, utilities.ConfigInfo.tars_config_file)
        # generate the tars config of the gateway-service
        tars_listen_port = node_config.tars_listen_port + node_index
        TarsConfigGenerator.generate_and_store_tars_conf(tars_config_tpl_path, tars_config_path,
                                                         node_config.app_name, node_config.service_name,
                                                         node_config.servant_object_list, ip, tars_listen_port)
