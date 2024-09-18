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
import com.webank.wedpr.sdk.jni.common.WeDPRSDKException;
import com.webank.wedpr.sdk.jni.generated.*;
import com.webank.wedpr.sdk.jni.generated.Error;
import com.webank.wedpr.sdk.jni.transport.IMessage;
import com.webank.wedpr.sdk.jni.transport.IMessageBuilder;
import com.webank.wedpr.sdk.jni.transport.TransportConfig;
import com.webank.wedpr.sdk.jni.transport.WeDPRTransport;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageDispatcherCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageErrorCallback;
import java.math.BigInteger;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TransportImpl implements WeDPRTransport {
    private static final Logger logger = LoggerFactory.getLogger(TransportImpl.class);
    // the created transport
    private final Transport transport;
    private final TransportConfig transportConfig;

    public static WeDPRTransport build(TransportConfig transportConfig) {
        return new TransportImpl(
                TransportConfig.getTransportBuilder()
                        .buildProTransport(transportConfig.getFrontConfig()),
                transportConfig);
    }

    protected TransportImpl(Transport transport, TransportConfig transportConfig) {
        logger.info("Build Transport, config: {}", transportConfig.toString());
        this.transport = transport;
        this.transport.disOwnMemory();
        this.transportConfig = transportConfig;
    }

    @Override
    public void start() {
        logger.info("start the transport");
        this.transport.start();
    }

    @Override
    public void stop() {
        logger.info("stop the transport");
        this.transport.stop();
    }

    /**
     * TODO: register the component
     *
     * @param component the component used to router
     * @throws Exception failed case
     */
    @Override
    public void registerComponent(String component) throws Exception {
        // Note: the front must exist after Transport created
    }

    /**
     * TODO: unregister the component
     *
     * @param component the component used to route
     * @throws Exception failed case
     */
    @Override
    public void unRegisterComponent(String component) throws Exception {}

    /**
     * register the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    @Override
    public void registerTopic(String topic) throws Exception {
        this.transport.getFront().registerTopic(topic);
    }

    /**
     * unRegister the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    @Override
    public void unRegisterTopic(String topic) throws Exception {
        this.transport.getFront().unRegisterTopic(topic);
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
        routeInfo.setDstNode(dstNode, BigInteger.valueOf(dstNode.length));
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
        routeInfo.setDstInst(dstInst);
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

    @Override
    public void asyncSendResponse(
            byte[] dstNode,
            String traceID,
            byte[] payload,
            int seq,
            MessageErrorCallback errorCallback) {
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
    @Override
    public void removeTopic(String topic) throws WeDPRSDKException {
        Error result = this.transport.getFront().unRegisterTopic(topic);
        Common.checkResult("removeTopic", result);
    }

    //// the sync interfaces
    @Override
    public void pushByNodeID(String topic, byte[] dstNodeID, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException {
        MessageOptionalHeader routeInfo =
                IMessageBuilder.buildRouteInfo(this.transport.routeInfoBuilder(), topic);
        routeInfo.setDstNode(dstNodeID, BigInteger.valueOf(dstNodeID.length));
        Error result =
                this.transport
                        .getFront()
                        .push(
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
        routeInfo.setDstInst(dstInst);
        routeInfo.setComponentType(component);
        Error result =
                this.transport
                        .getFront()
                        .push(
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
        routeInfo.setDstInst(dstInst);
        Error result =
                this.transport
                        .getFront()
                        .push(
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
        Message msg = this.transport.getFront().pop(topic, timeout);
        if (msg == null) {
            throw new WeDPRSDKException(
                    Constant.FAILED, "Try to receive msg with topic " + topic + " timeout!");
        }
        return IMessageBuilder.build(msg);
    }

    @Override
    public IMessage peek(String topic) {
        Message msg = this.transport.getFront().peek(topic);
        if (msg == null) {
            return null;
        }
        return IMessageBuilder.build(msg);
    }
}
