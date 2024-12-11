#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
import sys
from common import utilities
from config.binary_generator import BinaryGenerator
from config.cert_generator import CertGenerator
from config.shell_script_generator import ShellScriptGenerator


class PPCNodeConfigGenerator:
    """
    the ppc-node-config-generator
    """

    def __init__(self, config, output_dir):
        self.config = config
        self.output_dir = output_dir
        self.binary_name = utilities.ConfigInfo.ppc_node_binary_name
        self.service_type = utilities.ServiceInfo.node_service_type

    def generate_node_config(self):
        utilities.print_badge("* generate_node_config")
        # generate the ca cert for rpc
        ret = CertGenerator.generate_ca_cert(
            self.config.rpc_sm_ssl, self.__generate_ca_cert_path__())
        if ret is False:
            utilities.log_error(
                "* generate_node_config failed for generate ca error")
            return False
        for agency_config in self.config.agency_list.values():
            for node_config in agency_config.node_list.values():
                for ip_str in node_config.deploy_ip:
                    if ret is False:
                        return False
                    ip_array = ip_str.split(":")
                    ip = ip_array[0]
                    # generate the shell scripts for the given ip
                    ret = ShellScriptGenerator.generate_ip_shell_scripts(
                        self.__generate_ip_output_path__(node_config.agency_name, ip), "start_all.sh", "stop_all.sh")
                    node_count = 1
                    if len(ip_array) >= 2:
                        node_count = int(ip_array[1])
                    for node_index in range(node_count):
                        node_name = "node" + str(node_index)
                        if self.__generate_single_node_config__(node_config, ip, node_name, node_config.agency_name, node_index) is False:
                            return False
        utilities.print_badge("* generate_node_config success")
        return True

    def __generate_single_node_config__(self, node_config, ip, node_name, agency_name, node_index):
        utilities.print_badge("* generate node config for %s, ip: %s, agency: %s" %
                              (node_name, ip, agency_name))
        # copy the binary
        binary_path = os.path.join(
            self.config.env_config.binary_path, self.binary_name)
        dst_binary_path = os.path.join(
            self.__generate_ip_output_path__(agency_name, ip), self.binary_name)
        ret = BinaryGenerator.generate_binary(binary_path, dst_binary_path)
        if ret is False:
            return False
        # generate the node config
        node_path = self.__generate_node_path__(
            agency_name, ip, node_name)

        private_key_path = self.__generate_node_conf_path__(
            agency_name, ip, node_name)
        if self.__generate_single_node_inner_config__(utilities.ConfigInfo.node_config_tpl_path,
                                                      node_path,
                                                      private_key_path, node_config, ip,
                                                      node_index) is False:
            utilities.log_error("* generate node config, ip: %s failed" %
                                (ip))
            return False
        # generate the node cert(for rpc)
        ret = CertGenerator.generate_node_cert(self.config.rpc_sm_ssl, self.__generate_ca_cert_path__(
        ), self.__generate_node_conf_path__(agency_name, ip, node_name))
        if ret is False:
            utilities.log_error("* generate node config, ip: %s failed for generate rpc cert failed" %
                                (ip))
            return False
        ret = ShellScriptGenerator.generate_node_shell_scripts(
            self.__generate_node_path__(agency_name, ip, node_name), self.binary_name)
        if ret is False:
            return False
        utilities.print_badge("* generate node config %s, ip: %s.%s success" %
                              (node_name, agency_name, ip))
        return True

    def __generate_single_node_inner_config__(self, tpl_config_path, node_path, private_key_path,
                                              node_config, ip, node_index):
        config_content = utilities.load_config(tpl_config_path)
        utilities.log_debug(
            "__generate_single_node_inner_config__, load config.ini from %s" % tpl_config_path)
        # generate the private key
        (ret, node_id) = CertGenerator.generate_private_key(private_key_path)
        if ret is False:
            return False
        # load the common config
        self.__generate_common_config__(
            config_content, node_config)
        # load the rpc config
        self.__generate_rpc_config__(
            config_content, node_config.rpc_config, node_index)
        # load the transport config
        self.__generate_transport_config__(config_content,
                                           node_config, node_id, ip, node_index)
        # load the storage config
        self.__generate_storage_config__(
            config_content, node_config.storage_config)
        # load the hdfs_storage_config
        self.__generate_hdfs_storage_config__(node_path, utilities.ConfigInfo.krb5_config_tpl_path,
                                              config_content, node_config.hdfs_storage_config)
        # load the ra2018psi config
        self.__generate_ra2018psi_config__(
            config_content, node_config.ra2018psi_config)
        # store the config
        ini_config_output_path = os.path.join(
            node_path, utilities.ConfigInfo.config_ini_file)
        ret = utilities.store_config(
            config_content, "ini", ini_config_output_path, "config.ini")
        if ret is False:
            return False
        return True

    def __generate_ca_cert_path__(self):
        return os.path.join(self.output_dir, self.service_type, "ca")

    def __generate_node_path__(self, agency_name, ip, node_name):
        return os.path.join(self.output_dir, agency_name, ip, self.service_type,
                            node_name)

    def __generate_node_conf_path__(self, agency_name, ip, node_name):
        node_path = self.__generate_node_path__(agency_name, ip, node_name)
        return os.path.join(node_path, "conf")

    def __generate_ip_output_path__(self, agency_name, ip):
        return os.path.join(self.output_dir, agency_name, ip, self.service_type)

    def __generate_common_config__(self, config_content, node_config):
        """
        generate the common config
        """
        # the agency config
        config_content["agency"]["id"] = node_config.agency_name
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

    def __generate_hdfs_storage_config__(self, node_path, krb5_tpl_file_path, config_content, hdfs_storage_config):
        if hdfs_storage_config is None:
            return
        section_name = "hdfs_storage"
        config_content[section_name]["user"] = hdfs_storage_config.user
        config_content[section_name]["name_node"] = hdfs_storage_config.name_node
        config_content[section_name]["name_node_port"] = str(
            hdfs_storage_config.name_node_port)
        config_content[section_name]["token"] = hdfs_storage_config.token
        config_content[section_name]["enable_krb5_auth"] = hdfs_storage_config.enable_krb5_auth_str
        config_content[section_name]["auth_principal"] = hdfs_storage_config.auth_principal
        config_content[section_name]["auth_password"] = hdfs_storage_config.auth_password
        config_content[section_name]["ccache_path"] = hdfs_storage_config.ccache_path
        config_content[section_name]["krb5_conf_path"] = hdfs_storage_config.krb5_conf_path
        # copy krb5.conf to krb5_conf_path specified path
        dst_path = os.path.join(node_path, hdfs_storage_config.krb5_conf_path)
        if hdfs_storage_config.krb5_conf_path.startswith("/"):
            dst_path = hdfs_storage_config.krb5_conf_path
        command = "cp %s %s" % (krb5_tpl_file_path, dst_path)
        (ret, output) = utilities.execute_command_and_getoutput(command)
        if ret is False:
            utilities.log_error("copy krb5 configuration from %s to %s failed, error: %s") % (
                krb5_tpl_file_path, dst_path, output)
            return False
        return True

    def __generate_transport_config__(self, config_content,
                                      node_config, node_id,
                                      deploy_ip, node_index):
        """_summary_

        Args:
            ; the endpoint information
            listen_ip = 0.0.0.0
            listen_port = 18000
            host_ip = 
            ; the threadPoolSize
            thread_count = 4
            ; the gatewayService endpoint information
            gateway_target =  
            ; the components
            components =
            nodeid=
        """
        section = "transport"
        config_content[section]["listen_ip"] = node_config.grpc_listen_ip
        config_content[section]["listen_port"] = str(
            node_config.grpc_listen_port + node_index)
        config_content[section]["host_ip"] = deploy_ip
        config_content[section]["gateway_target"] = node_config.gateway_config.gateway_grpc_target
        config_content[section]["components"] = node_config.components
        config_content[section]["nodeid"] = node_id

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
