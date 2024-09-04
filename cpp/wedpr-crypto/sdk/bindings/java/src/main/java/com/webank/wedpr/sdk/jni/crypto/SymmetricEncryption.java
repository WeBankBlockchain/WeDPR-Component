/**
 * Copyright 2023 [wedpr]
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.webank.wedpr.sdk.jni.crypto;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.JniLibLoader;

public class SymmetricEncryption {
    static {
        JniLibLoader.loadJniLibrary();
    }

    public enum AlgorithmType {
        AES_128,
        AES_192,
        AES_256,
        TrippleDES,
        SM4
    }

    public enum OperationMode {
        ECB,
        CBC,
        CFB,
        OFB,
        CTR
    }

    public static native int keyBytes(int algorithmType, int mode) throws JniException;

    public static native byte[] generateKey(int algorithmType, int mode) throws JniException;

    public static native byte[] encrypt(
            int algorithmType, int mode, byte[] sk, byte[] iv, byte[] plain) throws JniException;

    public static native byte[] decrypt(
            int algorithmType, int mode, byte[] sk, byte[] iv, byte[] cipher) throws JniException;
}
