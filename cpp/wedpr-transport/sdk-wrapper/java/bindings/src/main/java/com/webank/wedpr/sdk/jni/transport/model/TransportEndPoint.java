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

import com.webank.wedpr.sdk.jni.generated.EndPoint;

public class TransportEndPoint {
    private final EndPoint endPoint;

    public TransportEndPoint(String hostIp, String listenIp, int listenPort) {
        this.endPoint = new EndPoint();
        setHostIP(hostIp);
        setListenIP(listenIp);
        setListenPort(listenPort);
    }

    public String getHostIP() {
        return this.endPoint.host();
    }

    protected void setHostIP(String hostIP) {
        this.endPoint.setHost(hostIP);
    }

    public String getListenIP() {
        return this.endPoint.listenIp();
    }

    protected void setListenIP(String listenIP) {
        this.endPoint.setListenIp(listenIP);
    }

    public Integer getListenPort() {
        return this.endPoint.port();
    }

    protected void setListenPort(Integer listenPort) {
        this.endPoint.setPort(listenPort);
    }

    public EndPoint getEndPoint() {
        return endPoint;
    }

    public EndPoint getNativeEndPoint() {
        return endPoint;
    }

    @Override
    public String toString() {
        return "TransportEndPoint{"
                + "hostIP='"
                + getHostIP()
                + '\''
                + ", listenIP='"
                + getListenIP()
                + '\''
                + ", listenPort="
                + getListenPort()
                + '}';
    }
}
