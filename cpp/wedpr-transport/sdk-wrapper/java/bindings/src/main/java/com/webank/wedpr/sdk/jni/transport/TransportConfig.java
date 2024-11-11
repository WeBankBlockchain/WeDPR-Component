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

import com.webank.wedpr.sdk.jni.common.JniLibLoader;
import com.webank.wedpr.sdk.jni.common.ObjectMapperFactory;
import com.webank.wedpr.sdk.jni.generated.FrontConfig;
import com.webank.wedpr.sdk.jni.generated.TransportBuilder;
import com.webank.wedpr.sdk.jni.transport.model.ServiceMeta;
import com.webank.wedpr.sdk.jni.transport.model.TransportEndPoint;
import com.webank.wedpr.sdk.jni.transport.model.TransportGrpcConfig;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** the transport config used to build the transport */
public class TransportConfig {
    private static final Logger logger = LoggerFactory.getLogger(TransportConfig.class);
    private static TransportBuilder transportBuilder;

    static {
        // load the jni
        JniLibLoader.loadJniLibrary();
        logger.info("loadJniLibrary success");
        createTransportBuilder();
        logger.info("createTransportBuilder success");
    }

    private static synchronized void createTransportBuilder() {
        if (transportBuilder != null) {
            logger.info("transportBuilder has already been created");
            return;
        }
        logger.info("init transportBuilder");
        transportBuilder = new TransportBuilder();
        transportBuilder.initLog("conf/wedpr_sdk_log_config.ini");
        logger.info("init transportBuilder success");
    }

    public static TransportBuilder getTransportBuilder() {
        return transportBuilder;
    }

    private final FrontConfig frontConfig;
    private List<String> components;
    private TransportEndPoint selfEndPoint;
    private ServiceMeta serviceMeta = new ServiceMeta();

    public TransportConfig(Integer threadPoolSize, String nodeID) {
        this.frontConfig = transportBuilder.buildConfig(threadPoolSize, nodeID);
        this.frontConfig.disOwnMemory();
        // set default grpcConfig
        setGrpcConfig(new TransportGrpcConfig());
    }

    public void setSelfEndPoint(TransportEndPoint endPoint) {
        if (endPoint == null) {
            return;
        }
        this.selfEndPoint = endPoint;
        this.frontConfig.setSelfEndPoint(endPoint.getEndPoint());
    }

    public void setGatewayTargets(String gatewayTargets) {
        this.frontConfig.setGatewayGrpcTarget(gatewayTargets);
    }

    public void setGrpcConfig(TransportGrpcConfig grpcConfig) {
        this.frontConfig.setGrpcConfig(grpcConfig.getGrpcConfig());
    }

    public void setComponents(List<String> components) {
        if (components == null || components.isEmpty()) {
            return;
        }
        this.components = components;
        for (String component : components) {
            this.frontConfig.addComponent(component);
        }
    }

    public void registerService(ServiceMeta serviceMeta) {
        if (serviceMeta != null) {
            this.serviceMeta = serviceMeta;
        }
    }

    public void registerService(String serviceName, String entryPoint) throws Exception {
        serviceMeta.addEntryPoint(new ServiceMeta.EntryPointMeta(serviceName, entryPoint));
        // update the meta
        this.frontConfig.setMeta(
                ObjectMapperFactory.getObjectMapper().writeValueAsString(serviceMeta));
    }

    public ServiceMeta getServiceMeta() {
        return this.serviceMeta;
    }

    public FrontConfig getFrontConfig() {
        return frontConfig;
    }

    public List<String> getComponents() {
        return components;
    }

    public TransportEndPoint getSelfEndPoint() {
        return selfEndPoint;
    }

    public Integer getThreadPoolSize() {
        return frontConfig.threadPoolSize();
    }

    public String getNodeID() {
        return frontConfig.nodeID();
    }

    public String getGatewayTargets() {
        return frontConfig.gatewayGrpcTarget();
    }

    @Override
    public String toString() {
        return "TransportConfig{"
                + "components="
                + components
                + ", selfEndPoint="
                + selfEndPoint
                + ", threadPoolSize="
                + getThreadPoolSize()
                + ", nodeID='"
                + getNodeID()
                + '\''
                + ", gatewayTargets='"
                + getGatewayTargets()
                + '\''
                + '}';
    }
}
