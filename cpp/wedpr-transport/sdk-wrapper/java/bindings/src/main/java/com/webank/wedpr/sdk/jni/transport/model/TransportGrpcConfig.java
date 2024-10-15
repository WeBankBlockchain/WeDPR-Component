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

package com.webank.wedpr.sdk.jni.transport.model;

import com.webank.wedpr.sdk.jni.generated.*;

public class TransportGrpcConfig {
    private final GrpcConfig grpcConfig = new GrpcConfig();
    public static final String defaultLoadBalancePolicy = "round_robin";

    public TransportGrpcConfig() {
        grpcConfig.setEnableHealthCheck(true);
        grpcConfig.setEnableDnslookup(false);
        grpcConfig.setLoadBalancePolicy(defaultLoadBalancePolicy);
    }

    public void setDefaultLoadBalancePolicy(String loadBalancePolicy) {
        grpcConfig.setLoadBalancePolicy(loadBalancePolicy);
    }

    public boolean isEnableHealthCheck() {
        return grpcConfig.enableHealthCheck();
    }

    public boolean isEnableDnslookup() {
        return grpcConfig.enableDnslookup();
    }

    public String getLoadBalancePolicy() {
        return grpcConfig.loadBalancePolicy();
    }

    public GrpcConfig getGrpcConfig() {
        return grpcConfig;
    }

    @Override
    public String toString() {
        return "TransportGrpcConfig{"
                + "enableHealthCheck="
                + isEnableHealthCheck()
                + ", enableDnslookup="
                + isEnableDnslookup()
                + ", loadBalancePolicy='"
                + getLoadBalancePolicy()
                + '\''
                + '}';
    }
}
