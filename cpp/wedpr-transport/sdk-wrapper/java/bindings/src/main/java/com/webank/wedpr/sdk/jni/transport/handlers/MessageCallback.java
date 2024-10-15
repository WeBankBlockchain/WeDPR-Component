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

package com.webank.wedpr.sdk.jni.transport.handlers;

import com.webank.wedpr.sdk.jni.generated.Error;
import com.webank.wedpr.sdk.jni.generated.IMessageHandler;
import com.webank.wedpr.sdk.jni.generated.Message;
import com.webank.wedpr.sdk.jni.generated.SendResponseHandler;
import com.webank.wedpr.sdk.jni.transport.IMessage;
import com.webank.wedpr.sdk.jni.transport.IMessageBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class MessageCallback extends IMessageHandler {
    private static final Logger logger = LoggerFactory.getLogger(MessageCallback.class);

    public abstract void onMessage(
            Error error, IMessage message, SendResponseHandler sendResponseHandler);

    // release the ownership to c++, in case of it's released by the jvm
    protected void finalize() {
        swigReleaseOwnership();
        delete();
    }

    @Override
    public void onMessage(Error e, Message msg, SendResponseHandler sendResponseHandler) {
        try {
            onMessage(e, IMessageBuilder.build(msg), sendResponseHandler);
        } catch (Exception exception) {
            logger.warn("onMessage exception, msg: {}, e:", msg.toString(), exception);
        }
    }
}
