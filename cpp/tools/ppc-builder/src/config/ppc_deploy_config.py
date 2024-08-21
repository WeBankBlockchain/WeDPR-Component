#!/usr/bin/python
# -*- coding: UTF-8 -*-

from common import utilities


class TarsConfig:
    """
    the tars config
    """

    def __init__(self, config):
        self.config = config
        self.app_name = utilities.get_value(
            self.config, "tars", "app_name", None, True)
        self.binary_path = utilities.get_value(
            self.config, "tars", "tars_pkg_dir", None, True)


class PeerInfo:
    def __init__(self, agency, endpoints):
        self.agency = agency
        self.endpoints = endpoints


class GatewayConfig:
    """
    the gateway config
    """

    def __init__(self, app_name, agency_name, holding_msg_minutes, config, config_section, must_exist):
        self.config = config
        self.config_section = config_section
        self.app_name = app_name
        self.holding_msg_minutes = holding_msg_minutes
        self.agency_name = agency_name
        self.service_name = "%s%s" % (
            self.agency_name, utilities.ServiceInfo.gateway_servant)
        self.servant_list = [utilities.ServiceInfo.gateway_servant]
        self.servant_object_list = utilities.ServiceInfo.gateway_servant_obj

        # the deploy_ip
        self.deploy_ip = utilities.get_item_value(
            self.config, "deploy_ip", None, must_exist, config_section)
        # the listen_ip
        self.listen_ip = utilities.get_item_value(
            self.config, "listen_ip", "0.0.0.0", False, config_section)
        # the listen_port
        self.listen_port = utilities.get_item_value(
            self.config, "listen_port", None, must_exist, config_section)
        # the thread count
        self.thread_count = utilities.get_item_value(
            self.config, "thread_count", 4, False, config_section)
        # the peers
        self.peers = []
        peers = utilities.get_item_value(
            self.config, "peers", None, must_exist, config_section)
        for peer in peers:
            agency = utilities.get_item_value(
                peer, "agency", None, must_exist, "[[peers]]")
            endpoints = utilities.get_item_value(
                peer, "endpoints", None, must_exist, "[[peers]]")
            self.peers.append(PeerInfo(agency, endpoints))

        self.cache_type = utilities.get_item_value(
            self.config, "cache_type", 0, must_exist, config_section)
        self.cache_proxy = utilities.get_item_value(
            self.config, "cache_proxy", None, must_exist, config_section)
        self.cache_obServer = utilities.get_item_value(
            self.config, "cache_obServer", None, must_exist, config_section)
        self.cache_cluster = utilities.get_item_value(
            self.config, "cache_cluster", None, must_exist, config_section)
        self.cache_host = utilities.get_item_value(
            self.config, "cache_host", None, must_exist, config_section)
        self.cache_port = utilities.get_item_value(
            self.config, "cache_port", None, must_exist, config_section)
        self.cache_password = utilities.get_item_value(
            self.config, "cache_password", "", must_exist, config_section)
        self.cache_database = utilities.get_item_value(
            self.config, "cache_database", None, must_exist, config_section)
        self.cache_pool_size = utilities.get_item_value(
            self.config, "cache_pool_size", 16, False, config_section)
        self.cache_connection_timeout = utilities.get_item_value(
            self.config, "cache_connection_timeout", 500, False, config_section)
        self.cache_socket_timeout = utilities.get_item_value(
            self.config, "cache_socket_timeout", 500, False, config_section)
        # the tars_listen_ip
        self.tars_listen_ip = utilities.get_item_value(
            self.config, "tars_listen_ip", "0.0.0.0", False, config_section)
        # the tars_listen_port
        self.tars_listen_port = utilities.get_item_value(
            self.config, "tars_listen_port", None, must_exist, config_section)


class RpcConfig:
    """
    the rpc config
    """

    def __init__(self, config, config_section, must_exist):
        self.config = config
        self.config_section = config_section
        self.listen_ip = utilities.get_item_value(
            self.config, "listen_ip", "0.0.0.0", False, config_section)
        self.listen_port = utilities.get_item_value(
            self.config, "listen_port", None, must_exist, config_section)
        self.thread_count = utilities.get_item_value(
            self.config, "thread_count", 4, False, config_section)


class StorageConfig:
    """
    the sql storage config
    """

    def __init__(self, config, config_section, must_exist):
        self.config = config
        self.config_section = config_section
        # the mysql configuration
        self.host = utilities.get_item_value(
            self.config, "host", None, must_exist, config_section)
        self.port = utilities.get_item_value(
            self.config, "port", None, must_exist, config_section)
        self.user = utilities.get_item_value(
            self.config, "user", None, must_exist, config_section)
        self.password = utilities.get_item_value(
            self.config, "password", None, must_exist, config_section)
        self.database = utilities.get_item_value(
            self.config, "database", None, must_exist, config_section)


class HDFSStorageConfig:
    """
    the hdfs storage config
    """

    def __init__(self, config, config_section, must_exist):
        self.config = config
        self.config_section = config_section
        # the hdfs configuration
        self.user = utilities.get_item_value(
            self.config, "user", None, must_exist, config_section)
        self.name_node = utilities.get_item_value(
            self.config, "name_node", None, must_exist, config_section)
        self.name_node_port = utilities.get_item_value(
            self.config, "name_node_port", None, must_exist, config_section)
        self.token = utilities.get_item_value(
            self.config, "token", "", False, config_section)


class RA2018PSIConfig:
    """
    the ra2018-psi config
    """

    def __init__(self, config, config_section, must_exist):
        self.config = config
        self.config_section = config_section
        self.database = utilities.get_item_value(
            self.config, "database", None, must_exist, config_section)
        self.cuckoofilter_capacity = utilities.get_item_value(
            self.config, "cuckoofilter_capacity", 1, False, config_section)
        self.cuckoofilter_tagBits = utilities.get_item_value(
            self.config, "cuckoofilter_tagBits", 32, False, config_section)
        self.cuckoofilter_buckets_num = utilities.get_item_value(
            self.config, "cuckoofilter_buckets_num", 4, False, config_section)
        self.cuckoofilter_max_kick_out_count = utilities.get_item_value(
            self.config, "cuckoofilter_max_kick_out_count", 20, False, config_section)
        self.trash_bucket_size = utilities.get_item_value(
            self.config, "trash_bucket_size", 10000, False, config_section)
        self.cuckoofilter_cache_size = utilities.get_item_value(
            self.config, "cuckoofilter_cache_size", 256, False, config_section)
        self.psi_cache_size = utilities.get_item_value(
            self.config, "psi_cache_size", 1024, False, config_section)
        self.data_batch_size = utilities.get_item_value(
            self.config, "data_batch_size", -1, False, config_section)
        self.use_hdfs = utilities.get_item_value(
            self.config, "use_hdfs", False, False, config_section)


class NodeGatewayConfig:
    """
    the gateway config for the node
    """

    def __init__(self, app_name, agency_name, config, node_must_exists):
        self.config = config
        self.app_name = app_name
        self.agency_name = agency_name
        self.desc = "[agency.node]"
        self.endpoints = utilities.get_item_value(
            self.config, "tars_endpoints", None, node_must_exists, self.desc)
        # obtain the gateway name
        self.service_name = "%s.%s%s" % (
            self.app_name, self.agency_name, utilities.ServiceInfo.gateway_servant)


class NodeConfig:
    """
    the ppc-node config
    """

    def __init__(self, app_name, agency_name, holding_msg_minutes, config, must_exist):
        self.config = config
        self.section_name = "[[agency.node]]."
        self.holding_msg_minutes = holding_msg_minutes
        self.app_name = app_name
        # set the agency_name
        self.agency_name = agency_name
        # disable ra2018 or not, default enable the ra2018
        self.disable_ra2018 = utilities.get_item_value(
            self.config, "disable_ra2018", False, False, self.section_name)
        # the deploy_ip
        self.deploy_ip = utilities.get_item_value(
            self.config, "deploy_ip", None, must_exist, self.section_name)
        # the node_name
        self.node_name = utilities.get_item_value(
            self.config, "node_name", None, must_exist, self.section_name)
        # the tars_listen_ip
        self.tars_listen_ip = utilities.get_item_value(
            self.config, "tars_listen_ip", "0.0.0.0", False, self.section_name)
        # the tars_listen_port
        self.tars_listen_port = utilities.get_item_value(
            self.config, "tars_listen_port", None, must_exist, self.section_name)
        utilities.log_debug("load the node config success")

        # parse the rpc config
        utilities.log_debug("load the rpc config")
        rpc_config_section_name = "[[agency.node.rpc]]"
        rpc_config_object = utilities.get_item_value(
            self.config, "rpc", None, must_exist, rpc_config_section_name)
        self.rpc_config = None
        if rpc_config_object is not None:
            self.rpc_config = RpcConfig(
                rpc_config_object, rpc_config_section_name, must_exist)
        utilities.log_debug("load the rpc config success")

        # parse the ra2018-psi config
        utilities.log_debug("load the ra2018psi config")
        ra2018psi_config_section = "[[agency.node.ra2018psi]]"
        ra2018psi_config_object = utilities.get_item_value(
            self.config, "ra2018psi", None, must_exist, ra2018psi_config_section)
        self.ra2018psi_config = None
        if ra2018psi_config_object is not None:
            self.ra2018psi_config = RA2018PSIConfig(
                ra2018psi_config_object, ra2018psi_config_section, must_exist)
        utilities.log_debug("load the ra2018psi config success")
        # parse the storage config
        utilities.log_debug("load the sql storage config")
        storage_config_section = "[[agency.node.storage]]"
        storage_config_object = utilities.get_item_value(
            self.config, "storage", None, must_exist, storage_config_section)
        self.storage_config = None
        if storage_config_object is not None:
            self.storage_config = StorageConfig(
                storage_config_object, storage_config_section, must_exist)
        utilities.log_debug("load the sql storage success")
        # parse the hdfs storage config
        hdfs_storage_must_configured = False
        if self.ra2018psi_config is not None:
            hdfs_storage_must_configured = self.ra2018psi_config.use_hdfs
        utilities.log_debug("load the hdfs storage config")
        storage_config_section = "[[agency.node.hdfs_storage]]"
        hdfs_storage_config_object = utilities.get_item_value(
            self.config, "hdfs_storage", None, hdfs_storage_must_configured, storage_config_section)
        self.hdfs_storage_config = None
        if hdfs_storage_config_object is not None:
            self.hdfs_storage_config = HDFSStorageConfig(
                hdfs_storage_config_object, storage_config_section, hdfs_storage_must_configured)
        utilities.log_debug("load the hdfs storage success")
        # parse the gateway-inforamtion
        utilities.log_debug("load the gateway config")
        gateway_config_section = "[[agency.node.gateway]]"
        gateway_config_object = utilities.get_item_value(
            self.config, "gateway", None, must_exist, gateway_config_section)
        self.gateway_config = None
        if gateway_config_object is not None:
            self.gateway_config = NodeGatewayConfig(
                self.app_name, self.agency_name, gateway_config_object, must_exist)
        utilities.log_debug("load the gateway success")

        # set the server name
        self.service_name = "%s%s%s" % (
            self.agency_name, self.node_name, utilities.ServiceInfo.node_service_postfix)
        # set the servant name
        self.servant_list = utilities.ServiceInfo.node_servant
        self.servant_object_list = utilities.ServiceInfo.node_servant_object


class AgencyConfig:
    """
    the agency config
    """

    def __init__(self, app_name, config, gateway_must_exists, node_must_exists):
        self.app_name = app_name
        self.config = config
        self.section_name = "[[agency]]"
        # the agency-name
        self.agency_name = utilities.get_item_value(
            self.config, "name", None, True, self.section_name)
        #  the holding_msg_minutes
        self.holding_msg_minutes = utilities.get_item_value(
            self.config, "holding_msg_minutes", 30, False, self.section_name)
        # parse the gateway config
        utilities.log_debug("load the gateway config")
        gateway_config_section_name = "[agency.gateway]"
        gateway_config_object = utilities.get_item_value(
            self.config, "gateway", None, gateway_must_exists, gateway_config_section_name)
        self.gateway_config = None
        if gateway_config_object is not None:
            self.gateway_config = GatewayConfig(
                self.app_name, self.agency_name, self.holding_msg_minutes, gateway_config_object,
                gateway_config_section_name, gateway_must_exists)
        utilities.log_debug("load the gateway config success")

        # parse the node config
        utilities.log_debug("load the node config")
        node_config_section_name = "[[agency.node]]"
        node_config_list = utilities.get_item_value(
            self.config, "node", None, node_must_exists, node_config_section_name)
        self.node_list = {}
        # TODO: check the node-name
        for node_object in node_config_list:
            node_config = NodeConfig(
                self.app_name, self.agency_name, self.holding_msg_minutes, node_object, node_must_exists)
            self.node_list[node_config.node_name] = node_config
            utilities.log_debug(
                "load node config for %s success" % node_config.node_name)
        utilities.log_debug("load the node config success")


class PPCDeployConfig:
    """
    load all config from config.toml
    """

    def __init__(self, config, gateway_must_exists, node_must_exists):
        self.config = config
        # load the crypto config
        utilities.log_debug("load the crypto config")
        crypto_section = "crypto"
        self.gateway_disable_ssl = utilities.get_value(
            self.config, crypto_section, "gateway_disable_ssl", False, False)
        self.gateway_sm_ssl = utilities.get_value(
            self.config, crypto_section, "gateway_sm_ssl", False, False)
        # the rpc disable ssl or not
        self.rpc_disable_ssl = utilities.get_value(
            self.config, crypto_section, "rpc_disable_ssl", False, False)
        # the rpc use sm-ssl or not
        self.rpc_sm_ssl = utilities.get_value(
            self.config, crypto_section, "rpc_sm_ssl", False, False)
        self.sm_crypto = utilities.get_value(
            self.config, crypto_section, "sm_crypto", False, False)
        utilities.log_debug("load the crypto config success")
        # load the tars config
        self.tars_config = TarsConfig(self.config)
        # load the agency config
        # TODO: check duplicated case
        utilities.log_debug("load the agency config")
        self.agency_list = {}
        agency_list_object = utilities.get_item_value(
            self.config, "agency", None, False, "[[agency]]")
        for agency_object in agency_list_object:
            agency_config = AgencyConfig(
                self.tars_config.app_name, agency_object, gateway_must_exists, node_must_exists)
            self.agency_list[agency_config.agency_name] = agency_config
            utilities.log_debug(
                "load the agency config for %s success" % agency_config.agency_name)
        utilities.log_debug("load the agency config success")
