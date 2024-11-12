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

package com.webank.wedpr.sdk.jni.common;

import com.webank.wedpr.sdk.jni.generated.*;
import com.webank.wedpr.sdk.jni.generated.Error;

public class Common {
    public static void requireNotNull(String objectName, Object object) throws WeDPRSDKException {
        if (object == null) {
            throw new WeDPRSDKException("The object " + objectName + " must be not null!");
        }
    }

    public static void checkResult(String interfaceName, Error error) throws WeDPRSDKException {
        if (error == null || error.errorCode() == 0) {
            return;
        }
        throw new WeDPRSDKException(
                error.errorCode(),
                "call " + interfaceName + " failed for: " + error.errorMessage());
    }

    public static String getUrl(String url) {
        if (url.startsWith("http://")) {
            return url;
        }
        return String.format("http://%s", url);
    }
}
