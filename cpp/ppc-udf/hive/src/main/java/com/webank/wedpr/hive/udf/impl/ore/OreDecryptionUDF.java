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
import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.sdk.jni.crypto.FastOre;
import javax.xml.bind.DatatypeConverter;
import org.apache.commons.codec.Charsets;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.log4j.Logger;

/** @author caryliao 2023/11/2 */
@Description(
        name = "ore_dec",
        value = "decrypt ore cipher to plain value",
        extended =
                "Usage:ore_dec(columnName, hexSk) \n"
                        + "Example:select ore_dec(encrypted_name, '303132...') as user_name from t_user")
public class OreDecryptionUDF extends UDF {
    private static final Logger logger = Logger.getLogger(OreDecryptionUDF.class.getName());

    public String evaluate(final String hexCipher) {
        if (hexCipher == null) {
            return null;
        }
        KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
        return evaluate(hexCipher, keyConfig.getOrePrivateKey());
    }

    public String evaluate(final String hexCipher, String hexSk) {
        try {
            if (hexCipher == null) {
                return null;
            }
            return new String(
                            FastOre.decrypt4String(
                                    DatatypeConverter.parseHexBinary(hexSk),
                                    hexCipher.getBytes(Charsets.UTF_8),
                                    true),
                            Charsets.UTF_8)
                    .trim();
        } catch (Exception e) {
            logger.error("OreDecryptionUDF jni fail ", e);
            throw new DecryptionException("ore Decrypt jni fail ", e);
        }
    }
}
