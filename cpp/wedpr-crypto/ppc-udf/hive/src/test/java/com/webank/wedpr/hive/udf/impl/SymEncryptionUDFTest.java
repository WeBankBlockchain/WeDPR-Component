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

package com.webank.wedpr.hive.udf.impl;

import com.webank.wedpr.hive.udf.impl.symmetric.SymDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.symmetric.SymEncryptionUDF;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;
import org.apache.commons.codec.binary.Hex;
import org.junit.Assert;
import org.junit.Test;

public class SymEncryptionUDFTest
{
public String testSymmetricEncryptionUDFEvaluateImpl(String plainValue, int algorithmType, int mode, String hexIv, String hexPrivateKey)
{
    SymEncryptionUDF symEncryptionUDF = new SymEncryptionUDF();
    String cipher = symEncryptionUDF.evaluate(plainValue, algorithmType, mode, hexIv, hexPrivateKey);
    // decrypt
    SymDecryptionUDF symDecUDF = new SymDecryptionUDF();
    String decodedPlain = symDecUDF.evaluate(cipher, algorithmType, mode, hexIv, hexPrivateKey);
    Assert.assertTrue(decodedPlain.equals(plainValue));
    return cipher;
}

public void checkDeterministic(String plainValue, int algorithmType, int mode, String hexPrivateKey, String hexIv)
{
    String cipher = "";
    for(int i = 0; i < 20; i++) {
        String currentCipher = testSymmetricEncryptionUDFEvaluateImpl(plainValue, algorithmType, mode, hexIv, hexPrivateKey);
        if(i >= 1)
        {
            Assert.assertTrue(currentCipher.equals(cipher));
        }
        cipher = currentCipher;
    }
}

@Test
public void testSymmetricEncryptionUDFEvaluate() throws JniException {
    String hexIv = "AABBCC";
    // sm4-ctr
    byte[] privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal());
    String hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("abc", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), hexPrivateKey, hexIv);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), hexPrivateKey, hexIv);
    // sm4-cbc
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    // check deterministic for cbc mode
    checkDeterministic("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), hexPrivateKey, hexIv);
    // sm4-cfb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), hexPrivateKey, hexIv);
    //sm4-ecb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), hexPrivateKey, hexIv);
    // sm4-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), hexPrivateKey, hexIv);

    // AES-128-ctr
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), hexPrivateKey, hexIv);

    // AES-128-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), hexPrivateKey, hexIv);

    // AES-128-cfb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), hexPrivateKey, hexIv);

    // AES-128-ecb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), hexPrivateKey, hexIv);

    // AES-128-cbc
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    checkDeterministic("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), hexPrivateKey, hexIv);

    // AES-192-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), hexPrivateKey, hexIv);

    // AES-192-ctr
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), hexPrivateKey, hexIv);

    // AES-192-cfb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), hexPrivateKey, hexIv);

    // AES-192-ecb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), hexPrivateKey, hexIv);

    // AES-192-cbc
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    checkDeterministic("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), hexPrivateKey, hexIv);

    // AES-256-ctr
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), hexPrivateKey, hexIv);

    // AES-256-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), hexPrivateKey, hexIv);

    // AES-256-cfb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), hexPrivateKey, hexIv);

    // AES-256-ecb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), hexPrivateKey, hexIv);

    // AES-256-cbc
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    checkDeterministic("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), hexPrivateKey, hexIv);

    // 3des-cfb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), hexPrivateKey, hexIv);

    // 3des-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), hexPrivateKey, hexIv);

    // 3des-ofb
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    testSymmetricEncryptionUDFEvaluateImpl("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), hexPrivateKey, hexIv);

    // 3des-cbc
    privateKey = SymmetricEncryption.generateKey(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal());
    hexPrivateKey = Hex.encodeHexString(privateKey);
    checkDeterministic("中文abc@#@$中文@Sf中国", SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), hexPrivateKey, hexIv);
}
}