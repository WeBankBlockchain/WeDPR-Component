#!/usr/bin/python
# -*- coding: UTF-8 -*-
from common import utilities
from config.ppc_gateway_config_generator import PPCGatewayConfigGenerator
from config.ppc_node_config_generator import PPCNodeConfigGenerator
from config.ppc_deploy_config import PPCDeployConfig
from argparse import RawTextHelpFormatter
import sys
import toml
import os
import argparse


def parse_command():
    help_info = "examples:\n * generate node config:\t python3 build_ppc.py -t node\n * generate gateway config:\t python3 build_ppc.py -t gateway\n * generate gateway config:\t python3 build_ppc.py -o genconfig -c config.toml -t gateway -d ppc-generated\n * generate node config:\t python3 build_ppc.py -o genconfig -c config.toml -t node -d ppc-generated"
    parser = argparse.ArgumentParser(
        prog=sys.argv[0], description=help_info, formatter_class=RawTextHelpFormatter, add_help=True)

    # the command option, now support genconfig/extend
    help_info = "[Optional] specify the command: \n* supported command list: %s\n" % (
        ''.join(utilities.CommandInfo.supported_command_list))
    parser.add_argument("-o", '--operation', help=help_info,
                        required=False, default=utilities.CommandInfo.generate_config)

    # config option
    help_info = "[Optional] the config file, default is config.toml\n"
    parser.add_argument(
        "-c", "--config", help=help_info, default="config.toml")
    # the output option
    help_info = "[Optional] the output path, default is pp-generated\n"
    parser.add_argument(
        "-d", "--output", help=help_info, default="ppc-generated")

    # the type option
    supported_service_type_str = ', '.join(
        utilities.ServiceInfo.supported_service_type)
    help_info = "[Required] the service type:\n* now support: %s \n" % (
        supported_service_type_str)
    parser.add_argument("-t", "--type", help=help_info, default="")
    args = parser.parse_args()
    return args


def generate_node_config(args, toml_config):
    """
    generate the node config
    """
    service_type = args.type
    # check the type
    if service_type not in utilities.ServiceInfo.supported_service_type:
        utilities.log_error("The service type must be " +
                            ', '.join(utilities.ServiceInfo.supported_service_type))
        sys.exit(-1)
    if service_type == utilities.ServiceInfo.node_service_type:
        utilities.log_debug("generate config for the ppc-node")
        config = PPCDeployConfig(toml_config, False, True)
        node_generator = PPCNodeConfigGenerator(config, args.output)
        ret = node_generator.generate_node_config()
        if ret is False:
            sys.exit(-1)
    if service_type == utilities.ServiceInfo.gateway_service_type:
        utilities.log_debug("generate config for the ppc-success")
        config = PPCDeployConfig(toml_config, True, False)
        gateway_generator = PPCGatewayConfigGenerator(config, args.output)
        ret = gateway_generator.generate_gateway_config()
        if ret is False:
            sys.exit(-1)


def execute_command(args):
    # check the config path
    if os.path.exists(args.config) is False:
        utilities.log_error("The config file %s not found!" % args.config)
        sys.exit(-1)
    # load the toml config
    toml_config = toml.load(args.config)
    # check the command
    command = args.operation
    if command not in utilities.CommandInfo.supported_command_list:
        utilities.log_error("The command must be " +
                            ', '.join(utilities.CommandInfo.supported_command_list))
        sys.exit(-1)
    if command == utilities.CommandInfo.generate_config:
        generate_node_config(args, toml_config)
        return
    # TODO: implement expand
    if command == utilities.CommandInfo.extend:
        utilities.log_error("unimplemented command %s" % command)
