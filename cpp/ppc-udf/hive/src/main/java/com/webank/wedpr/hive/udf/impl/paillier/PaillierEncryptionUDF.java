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
import com.webank.wedpr.hive.udf.exceptions.EncryptionException;
import com.webank.wedpr.hive.udf.exceptions.KeyException;
import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecException;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.common.JniException;
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
 * @date 2023/12/2
 */
@Description(
        name = "paillier_enc",
        value = "encrypt plain value to paillier cipher",
        extended =
                "Usage:paillier_enc(columnName, hexPk) \n"
                        + "Example:select paillier_enc(encrypted_salary,'0000080001...')")
public class PaillierEncryptionUDF extends UDF {
    private static final Logger logger = LoggerFactory.getLogger(PaillierEncryptionUDF.class);
    private static long publicKeyPointer = 0L;
    private static final NumberCodec codec = new NumberCodecImpl();

    static {
        try {
            KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
            publicKeyPointer =
                    NativePaillier.loadPublicKey(
                            DatatypeConverter.parseHexBinary(keyConfig.getPaillierPk()));
        } catch (Exception e) {
            throw new KeyException(
                    "Load public key for paillier exception, error: " + e.getMessage(), e);
        }
    }

    /**
     * paillier_enc(columnName, hexPk)
     *
     * @param plainValue
     * @return
     */
    public Text evaluate(final Text plainValue) {
        if (plainValue == null) {
            return null;
        }
        try {
            BigDecimal value = new BigDecimal(plainValue.toString());
            FloatingPointNumber plainValueFp = codec.encode(value);
            byte[] cipherValue =
                    NativeFloatingPointPaillier.encrypt(
                            plainValueFp.getSignificantBytes(),
                            plainValueFp.getExponent(),
                            publicKeyPointer);
            return new Text(DatatypeConverter.printHexBinary(cipherValue));
        } catch (NumberCodecException | JniException e) {
            logger.error("Paillier Encrypt BigDecimal jni fail ", e);
            throw new EncryptionException("WeFec Paillier Encrypt BigDecimal jni fail ", e);
        }
    }
}
