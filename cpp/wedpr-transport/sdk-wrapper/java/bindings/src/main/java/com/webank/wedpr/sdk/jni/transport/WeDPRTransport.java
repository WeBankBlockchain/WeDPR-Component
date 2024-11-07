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

package com.webank.wedpr.sdk.jni.transport;

import com.webank.wedpr.sdk.jni.common.WeDPRSDKException;
import com.webank.wedpr.sdk.jni.transport.handlers.GetPeersCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageDispatcherCallback;
import com.webank.wedpr.sdk.jni.transport.handlers.MessageErrorCallback;
import com.webank.wedpr.sdk.jni.transport.impl.RouteType;
import com.webank.wedpr.sdk.jni.transport.model.ServiceMeta;
import java.util.List;

public interface WeDPRTransport {
    // start the transport
    void start();
    // stop the transport
    void stop();
    /**
     * register the component
     *
     * @param component the component used to router
     * @throws Exception failed case
     */
    void registerComponent(String component) throws Exception;

    /**
     * unregister the component
     *
     * @param component the component used to route
     * @throws Exception failed case
     */
    void unRegisterComponent(String component) throws Exception;

    /**
     * register the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    void registerTopic(String topic) throws Exception;

    /**
     * unRegister the topic
     *
     * @param topic the topic used to route
     * @throws Exception failed case
     */
    void unRegisterTopic(String topic) throws Exception;

    /**
     * register handlers according to component
     *
     * @param component the component of the message should handled by the given callback
     * @param messageDispatcherCallback the message callback
     */
    void registerComponentHandler(
            String component, MessageDispatcherCallback messageDispatcherCallback);

    /**
     * async get peers information
     *
     * @param handler the handler that handle the peersInfo
     */
    void asyncGetPeers(GetPeersCallback handler);

    //// the async interfaces
    /**
     * register the message handler
     *
     * @param topic the topic
     * @param messageHandler the message handler
     */
    void registerTopicHandler(String topic, MessageDispatcherCallback messageHandler);

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
    void asyncSendMessageByNodeID(
            String topic,
            byte[] dstNode,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback);

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
    void asyncSendMessageByAgency(
            String topic,
            String agency,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback);

    void asyncSendMessageByComponent(
            String topic,
            String dstInst,
            String component,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback);

    /**
     * send message by the topic(will register firstly)
     *
     * @param topic the topic(used to route too
     * @param payload the payload(the payload)
     * @param seq the seq(the seq)
     * @param timeout the timeout
     * @param errorCallback the handler
     */
    void asyncSendMessageByTopic(
            String topic,
            String dstInst,
            byte[] payload,
            int seq,
            int timeout,
            MessageErrorCallback errorCallback,
            MessageCallback msgCallback);

    /**
     * send response to given node
     *
     * @param dstNode the node that should receive the response
     * @param traceID the traceID of the response
     * @param payload the payload
     * @param seq the seq
     * @param errorCallback the error callback
     */
    void asyncSendResponse(
            byte[] dstNode,
            String traceID,
            byte[] payload,
            int seq,
            MessageErrorCallback errorCallback);

    /** @param topic the topic to remove */
    void removeTopic(String topic) throws WeDPRSDKException;

    //// the sync interfaces
    void pushByNodeID(String topic, byte[] dstNodeID, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException;

    void pushByComponent(
            String topic, String dstInst, String component, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException;

    void pushByInst(String topic, String dstInst, int seq, byte[] payload, int timeout)
            throws WeDPRSDKException;

    IMessage pop(String topic, int timeout) throws WeDPRSDKException;

    IMessage peek(String topic);

    List<String> selectNodeListByPolicy(
            RouteType routeType, String dstInst, String dstComponent, String dstNode);

    List<ServiceMeta.EntryPointMeta> getAliveEntryPoints(String serviceName);

    void registerService(String serviceName, String entryPoint) throws Exception;

    public TransportConfig getTransportConfig();
}
