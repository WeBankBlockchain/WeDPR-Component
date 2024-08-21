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
package com.webank.wedpr.sdk.jni.homo;

import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.JniLibLoader;

public class NativeFloatingPointPaillier {
    static {
        JniLibLoader.loadJniLibrary();
    }

    public static native int maxCipherBytes(int keyBits) throws JniException;

    public static native byte[] encryptFast(byte[] significant, int exponent, long keyPair)
            throws JniException;

    public static native byte[] encryptFastWithoutPrecompute(
            byte[] significant, int exponent, byte[] sk, byte[] pk) throws JniException;

    public static native byte[] encrypt(byte[] significant, int exponent, long publicKey)
            throws JniException;

    public static native byte[] encryptWithoutPrecompute(
            byte[] significant, int exponent, byte[] pk) throws JniException;

    public static native FloatingPointNumber decrypt(byte[] cipher, long keyPair)
            throws JniException;

    public static native FloatingPointNumber decryptWithoutPrecompute(
            byte[] cipher, byte[] sk, byte[] pk) throws JniException;

    public static native byte[] add(byte[] c1, byte[] c2, long publicKey) throws JniException;

    public static native byte[] addWithoutPrecompute(byte[] c1, byte[] c2, byte[] pk)
            throws JniException;

    public static native byte[] sub(byte[] c1, byte[] c2, long publicKey) throws JniException;

    public static native byte[] subWithoutPrecompute(byte[] c1, byte[] c2, byte[] pkBytes)
            throws JniException;

    public static native byte[] scalaMul(
            byte[] significant, int exponent, byte[] cipher, long publicKey) throws JniException;

    public static native byte[] scalaMulWithoutPrecompute(
            byte[] significant, int exponent, byte[] cipher, byte[] pkBytes) throws JniException;
}
