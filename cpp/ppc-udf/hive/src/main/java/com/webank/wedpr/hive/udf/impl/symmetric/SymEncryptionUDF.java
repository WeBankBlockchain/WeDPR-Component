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
package com.webank.wedpr.hive.udf.impl.symmetric;

import com.webank.wedpr.hive.udf.config.ConfigLoader;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import com.webank.wedpr.hive.udf.exceptions.EncryptionException;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;
import javax.xml.bind.DatatypeConverter;
import org.apache.commons.codec.Charsets;
import org.apache.commons.codec.binary.Hex;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.log4j.Logger;

@Description(
        name = "sym_enc",
        value = "encrypt the plain-text into hex-cipher-data",
        extended =
                "Usage:sym_enc(columnName, algorithmType, mode, hexIv, hexSk) \n"
                        + "algorithmType(0:AES_128, 1:AES_192, 2:AES_256, 3:TrippleDES, 4:SM4), mode(0:ECB, 1:CBC, 2:CFB, 3:OFB, 4:CTR)\n"
                        + "Example: "
                        + "from t_user1 t1"
                        + "insert into t_user partition(dt=\"20231011\") select sym_enc(t1.id, 0, 0,'303132...', 'ce73b0...'),... ")
public class SymEncryptionUDF extends UDF {
    private static final Logger logger = Logger.getLogger(SymEncryptionUDF.class.getName());

    public String evaluate(final String plainData) {
        KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
        return evaluate(
                plainData,
                SymmetricEncryption.AlgorithmType.AES_256.ordinal(),
                SymmetricEncryption.OperationMode.CBC.ordinal(),
                keyConfig.getAesIv(),
                keyConfig.getAesPrivateKey());
    }

    public String evaluate(
            final String plainData,
            int algorithmType,
            int mode,
            String hexIv,
            String hexPrivateKey) {
        if (plainData == null) {
            return null;
        }
        if (hexPrivateKey == null) {
            logger.warn("Invalid empty private key");
            return plainData;
        }
        try {
            return Hex.encodeHexString(
                            SymmetricEncryption.encrypt(
                                    algorithmType,
                                    mode,
                                    DatatypeConverter.parseHexBinary(hexPrivateKey),
                                    DatatypeConverter.parseHexBinary(hexIv),
                                    plainData.getBytes(Charsets.UTF_8)))
                    .trim();
        } catch (JniException e) {
            logger.warn(
                    "Symmetric Encryption exception, algorithm: "
                            + algorithmType
                            + " , mode: "
                            + mode
                            + ", exception: "
                            + e);
            throw new EncryptionException("Symmetric Encryption exception", e);
        }
    }
}
