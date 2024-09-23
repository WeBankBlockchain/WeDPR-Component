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
import com.webank.wedpr.sdk.jni.generated.GetPeersInfoHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class GetPeersCallback extends GetPeersInfoHandler {
    private static final Logger logger = LoggerFactory.getLogger(GetPeersCallback.class);

    public abstract void onPeers(Error e, String peersInfo);

    public void onPeersInfo(Error e, String peersInfo) {
        try {
            onPeers(e, peersInfo);
        } catch (Exception exception) {
            logger.warn(
                    "onPeersInfo exception, result: {}, peersInfo: {}, e: ",
                    e.toString(),
                    peersInfo,
                    exception);
        }
    }

    // release the ownership to c++, in case of it's released by the jvm
    @Override
    protected void finalize() {
        swigReleaseOwnership();
        delete();
    }
}
