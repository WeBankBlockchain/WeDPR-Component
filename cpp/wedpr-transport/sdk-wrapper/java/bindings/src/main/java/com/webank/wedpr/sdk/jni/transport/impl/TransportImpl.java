/*
 * Copyright 2017-2025  [webank-wedpr]
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */

package com.webank.wedpr.sdk.jni.transport.impl;

import com.webank.wedpr.sdk.jni.common.Common;
import com.webank.wedpr.sdk.jni.common.Constant;
import com.webank.wedpr.sdk.jni.common.ObjectMapperFactory;
import com.webank.wedpr.sdk.jni.common.WeDPRSDKException;
import com.webank.wedpr.sdk.jni.generated.*;
import com.webank.wedpr.sdk.jni.generated.Error;
import com.webank.wedpr.sdk.jni.transport.IMessage;
import com.webank.wedpr.sdk.jni.transport.IMessageBuilder;
import com.webank.wedpr.sdk.jni.transport.TransportConfig;
import com.webank.wedpr.sdk.jni.transport.WeDPRTransport;
import com.webank.wedpr.sdk.jni.transport.handlers.GetPeersCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageDispatcherCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageErrorCallback;
import com.webank.wedpr.sdk.jni.transport.model.ServiceMeta;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;
import lombok.SneakyThrows;
import org.apache.commons.lang3.StringUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TransportImpl implements WeDPRTransport {
    private static final Logger logger = LoggerFactory.getLogger(TransportImpl.class);
    // the created transport
    private final Transport transport;
    private final TransportConfig transportConfig;

    public static WeDPRTransport build(TransportConfig transportConfig) throws WeDPRSDKException {
        try {
            return new TransportImpl(
                    TransportConfig.getTransportBuilder()
                            .buildProTransport(transportConfig.getFrontConfig()),
                    transportConfig);
        } catch (Exception e) {
            logger.warn(
                    "build transport failed, transport config: {}, error: ",
                    transportConfig.toString(),
                    e);
            throw e;
        }
    }

    protected TransportImpl(Transport transport, TransportConfig transportConfig)
            throws WeDPRSDKException {
        logger.info("Build Transport, config: {}", transportConfig.toString());
        this.transport = transport;
        this.transport.disOwnMemory();
        this.transportConfig = transportConfig;
    }

    @Override
    public void start() {
        try {
            logger.info("start the transport");
            this.transport.start();
        } catch (Exception e) {
            logger.warn("start the transport failed, error: ", e);
            if (this.transport != null) {
                this.transport.stop();
            }
            throw e;
        }
    }

    @Override
    public void stop() {
        logger.info("stop the transport");
        this.transport.stop();
    }

    /**
     * @param component the component used to router
     * @throws Exception failed case
     */
    @Override
    public void registerComponent(String component) {
        this.transport.getFront().registerComponent(component);
    }

    /**
     * @param component the component used to route
     * @throws Exception failed case
     */
    @Override
    public void unRegisterComponent(String component) {
        this.transport.getFront().unRegisterComponent(component);
    }

    /**
     * register the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    @Override
    public void registerTopic(String topic) throws Exception {
        Error result = this.transport.getFront().registerTopic(topic);
        Common.checkResult("registerTopic", result);
    }

    /**
     * unRegister the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    @Override
    public void unRegisterTopic(String topic) throws Exception {
        Error result = this.transport.getFront().unRegisterTopic(topic);
        Common.checkResult("unRegisterTopic", result);
    }

    /**
     * register handlers according to component
     *
     * @param component the component of the message should handled by the given callback
     * @param messageDispatcherCallback the message callback
     */
    @Override
    public void registerComponentHandler(
            String component, MessageDispatcherCallback messageDispatcherCallback) {
        this.transport.getFront().register_msg_handler(component, messageDispatcherCallback);
    }

    /**
     * async get peers information
     *
     * @param handler the handler that handle the peersInfo
     */
    @Override
    public void asyncGetPeers(GetPeersCallback handler) {
        this.transport.getFront().asyncGetPeers(handler);
    }

    @Override
    public void registerTopicHandler(String topic, MessageDispatcherCallback messageHandler) {
        this.transport.getFront().register_topic_handler(topic, messageHandler);
    }

    /**
     * async send message by the nodeID
     *
     * @param topic the topic
     * @param dstNode the dstNode
     * @param payload the payload
     * @param seq the seq of the payload
     * @param timeout the timeout setting
     * @param errorCallback the handler called after receive the message related to the topic
     */
    @SneakyThrows(Exception.class)
    @Override
    public void asyncSendMessageByNodeID(
            String topic,
            byte[] dstNode,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback) {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (dstNode == null) {
            throw new WeDPRSDKException(
                    "asyncSendMessageByNodeID failed for the dstNode information is empty");
        }
        routeInfo.setDstNodeBuffer(dstNode, BigInteger.valueOf(dstNode.length));
        this.transport
                .getFront()
                .async_send_message(
                        RouteType.ROUTE_THROUGH_NODEID.ordinal(),
                        routeInfo,
                        payload,
                        BigInteger.valueOf(payload.length),
                        seq,
                        timeout,
                        errorCallback,
                        msgCallback);
    }

    /**
     * send message by the agency
     *
     * @param topic the topic
     * @param agency the agency
     * @param payload the payload
     * @param seq the seq
     * @param timeout the timeout
     * @param errorCallback the handler called after receive the message related to the topic
     */
    @SneakyThrows(Exception.class)
    @Override
    public void asyncSendMessageByAgency(
            String topic,
            String agency,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback) {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (StringUtils.isBlank(agency)) {
            throw new WeDPRSDKException(
                    "asyncSendMessageByAgency failed for the dstInst information is empty");
        }
        routeInfo.setDstInst(agency);
        this.transport
                .getFront()
                .async_send_message(
                        RouteType.ROUTE_THROUGH_AGENCY.ordinal(),
                        routeInfo,
                        payload,
                        BigInteger.valueOf(payload.length),
                        seq,
                        timeout,
                        errorCallback,
                        msgCallback);
    }

    @SneakyThrows(Exception.class)
    @Override
    public void asyncSendMessageByComponent(
            String topic,
            String dstInst,
            String component,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback) {
        // set the routeInfo
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        // Note: support not specify the dstInst
        if (StringUtils.isNotBlank(dstInst)) {
            routeInfo.setDstInst(dstInst);
        }
        if (StringUtils.isBlank(component)) {
            throw new WeDPRSDKException(
                    "asyncSendMessageByComponent failed for the component information is empty");
        }
        routeInfo.setComponentType(component);
        this.transport
                .getFront()
                .async_send_message(
                        RouteType.ROUTE_THROUGH_COMPONENT.ordinal(),
                        routeInfo,
                        payload,
                        BigInteger.valueOf(payload.length),
                        seq,
                        timeout,
                        errorCallback,
                        msgCallback);
    }

    /**
     * send message by the topic(will register firstly)
     *
     * @param topic the topic(used to route too
     * @param payload the payload(the payload)
     * @param seq the seq(the seq)
     * @param timeout the timeout
     * @param errorCallback the handler
     */
    @SneakyThrows(Exception.class)
    @Override
    public void asyncSendMessageByTopic(
            String topic,
            String dstInst,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback) {
        // set the routeInfo
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (StringUtils.isBlank(dstInst)) {
            throw new WeDPRSDKException("asyncSendMessageByTopic failed for the dstInst is empty");
        }
        routeInfo.setDstInst(dstInst);
        this.transport
                .getFront()
                .async_send_message(
                        RouteType.ROUTE_THROUGH_TOPIC.ordinal(),
                        routeInfo,
                        payload,
                        BigInteger.valueOf(payload.length),
                        seq,
                        timeout,
                        errorCallback,
                        msgCallback);
    }

    @SneakyThrows(Exception.class)
    @Override
    public void asyncSendResponse(
            byte[] dstNode,
            String traceID,
            byte[] payload,
            int seq,
            MessageErrorCallback errorCallback) {
        if (dstNode == null) {
            throw new WeDPRSDKException("asyncSendResponse failed for the dstNode is empty");
        }
        this.transport
                .getFront()
                .async_send_response(
                        dstNode,
                        BigInteger.valueOf(dstNode.length),
                        traceID,
                        payload,
                        BigInteger.valueOf(payload.length),
                        seq,
                        errorCallback);
    }

    /** @param topic the topic to remove */
    @SneakyThrows(Exception.class)
    @Override
    public void removeTopic(String topic) throws WeDPRSDKException {
        if (StringUtils.isBlank(topic)) {
            throw new WeDPRSDKException("removeTopic failed for the topic is empty");
        }
        Error result = this.transport.getFront().unRegisterTopic(topic);
        Common.checkResult("removeTopic", result);
    }

    //// the sync interfaces
    @Override
    public void pushByNodeID(String topic, byte[] dstNodeID, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (dstNodeID == null) {
            throw new WeDPRSDKException("pushByNodeID failed for the dstNode is empty");
        }
        routeInfo.setDstNodeBuffer(dstNodeID, BigInteger.valueOf(dstNodeID.length));
        Error result =
                this.transport
                        .getFront()
                        .push_msg(
                                RouteType.ROUTE_THROUGH_NODEID.ordinal(),
                                routeInfo,
                                payload,
                                BigInteger.valueOf(payload.length),
                                seq,
                                timeout);
        Common.checkResult("pushByNodeID", result);
    }

    @Override
    public void pushByComponent(
            String topic, String dstInst, String component, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (StringUtils.isNotBlank(dstInst)) {
            routeInfo.setDstInst(dstInst);
        }
        if (StringUtils.isBlank(component)) {
            throw new WeDPRSDKException("pushByComponent failed for the component is empty");
        }
        routeInfo.setComponentType(component);
        Error result =
                this.transport
                        .getFront()
                        .push_msg(
                                RouteType.ROUTE_THROUGH_COMPONENT.ordinal(),
                                routeInfo,
                                payload,
                                BigInteger.valueOf(payload.length),
                                seq,
                                timeout);
        Common.checkResult("pushByComponent", result);
    }

    @Override
    public void pushByInst(String topic, String dstInst, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        if (StringUtils.isBlank(dstInst)) {
            throw new WeDPRSDKException("pushByInst failed for the dstInst is empty");
        }
        routeInfo.setDstInst(dstInst);
        Error result =
                this.transport
                        .getFront()
                        .push_msg(
                                RouteType.ROUTE_THROUGH_TOPIC.ordinal(),
                                routeInfo,
                                payload,
                                BigInteger.valueOf(payload.length),
                                seq,
                                timeout);
        Common.checkResult("pushByInst", result);
    }

    @Override
    public IMessage pop(String topic, int timeout) throws WeDPRSDKException {
        if (StringUtils.isBlank(topic)) {
            throw new WeDPRSDKException("pop failed for the topic is empty");
        }
        Message msg = this.transport.getFront().pop(topic, timeout);
        if (msg == null) {
            throw new WeDPRSDKException(
                    Constant.FAILED, "Try to receive msg with topic " + topic + " timeout!");
        }
        return IMessageBuilder.build(msg);
    }

    @SneakyThrows(Exception.class)
    @Override
    public IMessage peek(String topic) {
        if (StringUtils.isBlank(topic)) {
            throw new WeDPRSDKException("peek failed for the topic is empty");
        }
        Message msg = this.transport.getFront().peek(topic);
        if (msg == null) {
            return null;
        }
        return IMessageBuilder.build(msg);
    }

    @Override
    public List<String> selectNodeListByPolicy(
            RouteType routeType, String dstInst, String dstComponent, String dstNode) {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder());
        if (StringUtils.isNotBlank(dstInst)) {
            routeInfo.setDstInst(dstInst);
        }
        if (StringUtils.isNotBlank(dstComponent)) {
            routeInfo.setComponentType(dstComponent);
        }
        if (StringUtils.isNotBlank(dstNode)) {
            routeInfo.setDstNodeBuffer(dstNode.getBytes(), BigInteger.valueOf(dstNode.length()));
        }
        StringVec result =
                this.transport
                        .getFront()
                        .selectNodesByRoutePolicy((short) routeType.ordinal(), routeInfo);
        if (result == null) {
            return null;
        }
        List<String> nodeList = new ArrayList<>();
        for (int i = 0; i < result.size(); i++) {
            nodeList.add(result.get(i));
        }
        return nodeList;
    }

    private void parseServiceMeta(
            List<ServiceMeta.EntryPointMeta> entryPointInfos,
            String serviceName,
            String meta,
            StringVec componentsVec) {
        try {
            if (StringUtils.isBlank(meta)) {
                return;
            }
            ServiceMeta serviceMeta =
                    ObjectMapperFactory.getObjectMapper().readValue(meta, ServiceMeta.class);
            if (serviceMeta.getServiceInfos() == null || serviceMeta.getServiceInfos().isEmpty()) {
                return;
            }
            for (ServiceMeta.EntryPointMeta entryPointMeta : serviceMeta.getServiceInfos()) {
                if (entryPointMeta.getServiceName().equalsIgnoreCase(serviceName)) {
                    entryPointInfos.add(entryPointMeta);
                    List<String> components = new ArrayList<>();
                    for (int i = 0; i < componentsVec.size(); i++) {
                        entryPointMeta.getComponents().add(componentsVec.get(i));
                    }
                }
            }
        } catch (Exception e) {
            logger.warn("parseServiceMeta exception, meta: {}", meta, e);
        }
    }

    @Override
    public List<ServiceMeta.EntryPointMeta> getAliveEntryPoints(String serviceName) {
        NodeInfoVec nodeInfoList = this.transport.getFront().getNodeDiscovery().getAliveNodeList();
        List<ServiceMeta.EntryPointMeta> result = new ArrayList<>();
        for (int i = 0; i < nodeInfoList.size(); i++) {
            parseServiceMeta(
                    result,
                    serviceName,
                    nodeInfoList.get(i).meta(),
                    nodeInfoList.get(i).copiedComponents());
        }
        return result;
    }

    @Override
    public void registerService(String serviceName, String entryPoint) throws Exception {
        transportConfig
                .getServiceMeta()
                .addEntryPoint(new ServiceMeta.EntryPointMeta(serviceName, entryPoint));
        // update the meta
        this.transport
                .getFront()
                .updateMetaInfo(
                        ObjectMapperFactory.getObjectMapper()
                                .writeValueAsString(transportConfig.getServiceMeta()));
    }

    @Override
    public TransportConfig getTransportConfig() {
        return transportConfig;
    }
}
