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
import com.webank.wedpr.sdk.jni.generated.ErrorCallback;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class MessageErrorCallback extends ErrorCallback {
    private static final Logger logger = LoggerFactory.getLogger(MessageErrorCallback.class);

    public abstract void onErrorResult(Error error);

    public void onError(Error error) {
        try {
            onErrorResult(error);
        } catch (Exception e) {
            logger.warn("onError exception, error: {}, e:", error.toString(), e);
        }
    }
    // release the ownership to c++, in case of it's released by the jvm
    @Override
    protected void finalize() {
        swigReleaseOwnership();
        delete();
    }
}
