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
import com.webank.wedpr.sdk.jni.generated.Message;
import com.webank.wedpr.sdk.jni.generated.MessageOptionalHeader;
import com.webank.wedpr.sdk.jni.generated.MessageOptionalHeaderBuilder;
import com.webank.wedpr.sdk.jni.transport.impl.MessageImpl;
import lombok.SneakyThrows;
import org.apache.commons.lang3.StringUtils;

public class IMessageBuilder {
    public static IMessage build(Message msg) {
        if (msg == null) {
            return null;
        }
        return new MessageImpl(msg);
    }

    @SneakyThrows(Exception.class)
    public static MessageOptionalHeader buildRouteInfo(
            MessageOptionalHeaderBuilder routeInfoBuilder, String topic) {
        if (StringUtils.isBlank(topic)) {
            throw new WeDPRSDKException("buildRouteInfo for invalid empty topic!");
        }
        // return the ownership to cpp, since it is created by cpp
        MessageOptionalHeader routeInfo = routeInfoBuilder.build();
        routeInfo.setTopic(topic);
        routeInfo.disOwnMemory();
        return routeInfo;
    }
}
