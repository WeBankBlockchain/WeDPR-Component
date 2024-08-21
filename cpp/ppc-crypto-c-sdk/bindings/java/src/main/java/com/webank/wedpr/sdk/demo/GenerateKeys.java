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
package com.webank.wedpr.sdk.demo;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.Utilities;
import com.webank.wedpr.sdk.jni.crypto.FastOre;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
import com.webank.wedpr.sdk.jni.homo.NativePaillier;

public class GenerateKeys {
    public static void main(String[] args) throws JniException {
        // AES-128-CBC
        System.out.println("Generate key for AES-128-CBC:");
        byte[] sk =
                SymmetricEncryption.generateKey(
                        SymmetricEncryption.AlgorithmType.AES_128.ordinal(),
                        SymmetricEncryption.OperationMode.CBC.ordinal());
        System.out.println(Utilities.bytesToHex(sk));
        // AES-256-CBC
        System.out.println("Generate key for AES-256-CBC:");
        sk =
                SymmetricEncryption.generateKey(
                        SymmetricEncryption.AlgorithmType.AES_256.ordinal(),
                        SymmetricEncryption.OperationMode.CBC.ordinal());
        System.out.println(Utilities.bytesToHex(sk));
        // ore
        System.out.println("Generate key for ORE:");
        sk = FastOre.generateKey();
        System.out.println(Utilities.bytesToHex(sk));
        // ihc-128
        System.out.println("Generate key for IHC-128:");
        sk = NativeFloatingIhc.generateKey(NativeFloatingIhc.mode.IHC_128.ordinal());
        System.out.println(Utilities.bytesToHex(sk));
        // ihc-256
        System.out.println("Generate key for IHC-256:");
        sk = NativeFloatingIhc.generateKey(NativeFloatingIhc.mode.IHC_256.ordinal());
        System.out.println(Utilities.bytesToHex(sk));
        // paillier
        System.out.println("Generate key for paillier-2048:");
        long keyPair = NativePaillier.generateKeyPair(2048);
        System.out.println(
                "sk: "
                        + Utilities.bytesToHex(
                                NativePaillier.getPrivateKeyBytesFromKeyPair(keyPair)));
        System.out.println(
                "pk: "
                        + Utilities.bytesToHex(
                                NativePaillier.getPublicKeyBytesFromKeyPair(keyPair)));
        NativePaillier.freeKeyPair(keyPair);
    }
}
