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
import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;
import javax.xml.bind.DatatypeConverter;
import org.apache.commons.codec.Charsets;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.log4j.Logger;

/** @author caryliao 2023/11/1 */
@Description(
        name = "sym_dec",
        value = "decrypt symmetric cipher to plain value",
        extended =
                "Usage:sym_dec(columnName, algorithmType, mode, hexIv, hexSk) \n"
                        + "algorithmType(0:AES_128, 1:AES_192, 2:AES_256, 3:TrippleDES, 4:SM4), mode(0:ECB, 1:CBC, 2:CFB, 3:OFB, 4:CTR)\n"
                        + "Example:select sym_dec(encrypted_name, 0, 0,'303132...', 'ce73b0...') as user_name from t_user")
public class SymDecryptionUDF extends UDF {
    private static final Logger logger = Logger.getLogger(SymDecryptionUDF.class.getName());

    public String evaluate(final String hexCipher) {
        KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
        return evaluate(
                hexCipher,
                SymmetricEncryption.AlgorithmType.AES_256.ordinal(),
                SymmetricEncryption.OperationMode.CBC.ordinal(),
                keyConfig.getAesIv(),
                keyConfig.getAesPrivateKey());
    }
    /* @param hexCipher
     * @param algorithmType
     * @param mode
     * @param hexIv
     * @param hexSk
     * @return
     */
    public String evaluate(
            final String hexCipher, int algorithmType, int mode, String hexIv, String hexSk) {
        try {
            if (hexCipher == null) {
                return null;
            }
            return new String(
                            SymmetricEncryption.decrypt(
                                    algorithmType,
                                    mode,
                                    DatatypeConverter.parseHexBinary(hexSk),
                                    DatatypeConverter.parseHexBinary(hexIv),
                                    DatatypeConverter.parseHexBinary(hexCipher)),
                            Charsets.UTF_8)
                    .trim();
        } catch (Exception e) {
            logger.error("Sym Decrypt fail ", e);
            throw new DecryptionException("Sym Decrypt jni fail ", e);
        }
    }
}
