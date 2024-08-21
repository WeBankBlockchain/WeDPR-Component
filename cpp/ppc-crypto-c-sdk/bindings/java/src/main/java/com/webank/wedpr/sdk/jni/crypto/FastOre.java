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

public class FastOre {
    static {
        JniLibLoader.loadJniLibrary();
    }

    public static native int keyBytes() throws JniException;

    public static native long cipherSize(long plainDataSize, boolean hex) throws JniException;

    public static native long plainSize(long cipherDataSize, boolean hex) throws JniException;

    public static native byte[] generateKey() throws JniException;

    public static native byte[] encrypt4String(byte[] sk, byte[] plainData, boolean hexEncode)
            throws JniException;

    public static native byte[] decrypt4String(byte[] sk, byte[] cipherData, boolean hexEncode)
            throws JniException;

    public static native byte[] encrypt4Integer(byte[] sk, long plain, boolean hexEncode)
            throws JniException;

    public static native long decrypt4Integer(byte[] sk, byte[] cipherData, boolean hexEncode)
            throws JniException;

    public static native byte[] encrypt4Float(byte[] sk, byte[] plainData, boolean hexEncode)
            throws JniException;

    public static native byte[] decrypt4Float(byte[] sk, byte[] cipherData, boolean hexEncode)
            throws JniException;

    public static native int compare(byte[] cipher1, byte[] cipher2) throws JniException;
}
