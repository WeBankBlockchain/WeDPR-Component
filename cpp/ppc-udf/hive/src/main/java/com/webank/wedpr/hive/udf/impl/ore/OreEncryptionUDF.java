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
package com.webank.wedpr.hive.udf.impl.ore;

import com.webank.wedpr.hive.udf.config.ConfigLoader;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import com.webank.wedpr.hive.udf.exceptions.EncryptionException;
import com.webank.wedpr.sdk.jni.crypto.FastOre;
import javax.xml.bind.DatatypeConverter;
import org.apache.commons.codec.Charsets;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.log4j.Logger;

@Description(
        name = "ore_enc",
        value = "encrypt ore plain-text to cipher",
        extended =
                "Usage:ore_enc(columnName, hexSk) \n"
                        + "Example:select ore_enc(encrypted_name, '303132...') as user_name from t_user")
public class OreEncryptionUDF extends UDF {
    private static final Logger logger = Logger.getLogger(OreEncryptionUDF.class.getName());

    public String evaluate(final String plainData) {
        KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
        return evaluate(plainData, keyConfig.getOrePrivateKey());
    }

    public String evaluate(final String plainData, String hexSk) {
        try {
            if (plainData == null) {
                return null;
            }
            return new String(
                            FastOre.encrypt4String(
                                    DatatypeConverter.parseHexBinary(hexSk),
                                    plainData.getBytes(Charsets.UTF_8),
                                    true),
                            Charsets.UTF_8)
                    .trim();
        } catch (Exception e) {
            logger.warn("ORE Encryption exception", e);
            throw new EncryptionException("ORE Encryption exception", e);
        }
    }
}
