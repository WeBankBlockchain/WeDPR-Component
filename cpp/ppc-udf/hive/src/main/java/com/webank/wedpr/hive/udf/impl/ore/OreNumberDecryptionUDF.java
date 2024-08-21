/**
 * Copyright 2024 [wedpr]
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
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.crypto.FastOre;
import javax.xml.bind.DatatypeConverter;
import org.apache.commons.codec.Charsets;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.log4j.Logger;

public class OreNumberDecryptionUDF extends UDF {
    private static final Logger logger = Logger.getLogger(OreNumberDecryptionUDF.class.getName());
    private static final NumberCodec codec = new NumberCodecImpl();

    public String evaluate(String cipher) {
        KeyConfig keyConfig = ConfigLoader.config.getKeyConfig();
        try {
            if (cipher == null) {
                return null;
            }
            return codec.toBigDecimal(
                            FastOre.decrypt4Float(
                                    DatatypeConverter.parseHexBinary(keyConfig.getOrePrivateKey()),
                                    cipher.getBytes(Charsets.UTF_8),
                                    true))
                    .toPlainString();
        } catch (Exception e) {
            logger.error("OreNumberDecryptionUDF jni fail ", e);
            throw new DecryptionException("ore Decrypt jni fail ", e);
        }
    }
}
