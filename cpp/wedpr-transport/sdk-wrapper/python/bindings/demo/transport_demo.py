# -*- coding: utf-8 -*-
import sys
import os
# Note: here can't be refactored by autopep
root_path = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(root_path, "../"))
# Note: here can't be refactored by autopep

import time
from wedpr_python_gateway_sdk.transport.impl.transport_loader import TransportLoader
from wedpr_python_gateway_sdk.transport.impl.transport_config import TransportConfig
from wedpr_python_gateway_sdk.transport.impl.transport import Transport
from wedpr_python_gateway_sdk.transport.impl.transport import RouteType
import traceback
import argparse


def parse_args():
    parser = argparse.ArgumentParser(prog=sys.argv[0])
    parser.add_argument("-t", '--threadpool_size',
                        help='the threadpool size', default=4, required=True)
    parser.add_argument('--dst_inst',
                        help='the dist inst', default=None, required=True)
    parser.add_argument("-n", '--node_id',
                        help='the nodeID', required=False)
    parser.add_argument("-g", '--gateway_targets',
                        help='the gateway targets, e.g: ipv4:127.0.0.1:40620,127.0.0.1:40621', required=True)
    parser.add_argument("-i", '--host_ip',
                        help='the host ip, e.g.: 127.0.0.1', required=True)
    parser.add_argument("-p", "--listen_port",
                        help="the listen port", required=True)
    parser.add_argument("-d", "--dst_node",
                        help="the dst node", required=True)
    args = parser.parse_args()
    return args


def message_event_loop(args):
    transport_config = TransportConfig(
        int(args.threadpool_size), args.node_id, args.gateway_targets)
    transport_config.set_self_endpoint(
        args.host_ip, int(args.listen_port), "0.0.0.0")
    # def register_service_info(self, service, entrypoint):
    service_name = "python_service_demo"
    entrypoint = f"{args.host_ip}:{args.listen_port}"
    transport_config.register_service_info(service_name, entrypoint)
    service_name2 = "python_service_demo2"
    transport_config.register_service_info(service_name2, entrypoint)

    transport = TransportLoader.load(transport_config)
    print(f"Create transport success, config: {transport_config.desc()}")
    transport.start()
    component = "WEDPR_COMPONENT_TEST"
    transport.register_component(component)
    print(f"Start transport success")
    test_topic = "sync_message_event_loop_test"
    # register service after start
    service_name3 = "python_service_demo3"
    transport.register_service_info(service_name3, entrypoint)
    while Transport.should_exit is False:
        try:
            # fetch the alive entrypoints
            alive_endpoint = transport.get_alive_entrypoints(service_name)
            print(f"##### alive_endpoint: {alive_endpoint}")
            alive_endpoint2 = transport.get_alive_entrypoints(service_name2)
            print(f"##### alive_endpoint2: {alive_endpoint2}")
            alive_endpoint3 = transport.get_alive_entrypoints(service_name3)
            print(f"##### alive_endpoint3: {alive_endpoint3}")
            payload = b"test"
            transport.push_by_nodeid(topic=test_topic, dstNode=bytes(args.dst_node, encoding="utf-8"),
                                     seq=0, payload=payload, timeout=6000)
            msg = transport.pop(test_topic, timeout_ms=6000)
            if msg is None:
                print("Receive message timeout")
                continue
            print(
                f"Receive message: {msg.detail()}, buffer: {str(msg.get_payload())}")

            node_list = transport.select_node_list_by_route_policy(route_type=RouteType.ROUTE_THROUGH_COMPONENT,
                                                                   dst_inst=args.dst_inst,
                                                                   dst_component=component)
            if node_list is None:
                print(
                    f"####select_node_list_by_route_policy: not find component: {component}")
                continue
            for node in node_list:
                print(
                    f"##### select_node_list_by_route_policy result: {node}, component: {component}")

            selected_node = transport.select_node_by_route_policy(route_type=RouteType.ROUTE_THROUGH_COMPONENT,
                                                                  dst_inst=args.dst_inst,
                                                                  dst_component=component)
            if selected_node is None:
                print(
                    f"####select_node_by_route_policy: not find component: {component}")
            print(
                f"##### select_node_by_route_policy, selected_node: {selected_node}, component: {component}")
        except Exception as e:
            traceback.print_exc()
            print(f"exception: {e}")
        time.sleep(2)
    print(f"stop the transport")
    transport.stop()
    print(f"stop the transport successfully")


if __name__ == "__main__":
    args = parse_args()
    message_event_loop(args)
