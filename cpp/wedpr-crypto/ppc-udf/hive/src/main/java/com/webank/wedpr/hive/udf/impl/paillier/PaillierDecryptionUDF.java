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
package com.webank.wedpr.hive.udf.impl.paillier;

import com.webank.wedpr.hive.udf.config.ConfigLoader;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.hive.udf.exceptions.KeyException;
import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingPointPaillier;
import com.webank.wedpr.sdk.jni.homo.NativePaillier;
import java.math.BigDecimal;
import javax.xml.bind.DatatypeConverter;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.hadoop.io.Text;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author caryliao
 * @date 2023/10/31
 */
@Description(
        name = "paillier_dec",
        value = "decrypt paillier cipher to plain value",
        extended =
                "Usage:paillier_dec(columnName) \n"
                        + "Example:select paillier_dec(encrypted_salary)")
public class PaillierDecryptionUDF extends UDF {
    private static final Logger logger = LoggerFactory.getLogger(PaillierDecryptionUDF.class);

    private static long keyPairPointer = 0L;
    private static final NumberCodec codec = new NumberCodecImpl();

    static {
        try {
            KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
            keyPairPointer =
                    NativePaillier.loadKeyPair(
                            DatatypeConverter.parseHexBinary(keyConfig.getPaillierSk()),
                            DatatypeConverter.parseHexBinary(keyConfig.getPaillierPk()));
        } catch (Exception e) {
            throw new KeyException("Load key pair for paillier error", e);
        }
    }
    /**
     * paillier_dec(columnName)
     *
     * @param hexCipher
     * @return
     */
    public String evaluate(final Text hexCipher) {
        if (hexCipher == null) {
            return null;
        }
        try {
            FloatingPointNumber cipherValueFp =
                    NativeFloatingPointPaillier.decrypt(
                            DatatypeConverter.parseHexBinary(hexCipher.toString()), keyPairPointer);
            int scale = (cipherValueFp.getExponent() <= 0 ? -cipherValueFp.getExponent() : 0);
            return codec.decodeBigDecimal(cipherValueFp)
                    .setScale(scale, BigDecimal.ROUND_HALF_UP)
                    .toString();
        } catch (Exception e) {
            logger.error("Paillier Decrypt jni fail ", e);
            throw new DecryptionException("Paillier Decrypt jni fail ", e);
        }
    }
}
