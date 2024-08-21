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
package com.webank.wedpr.hive.udf.impl.ihc;

import com.webank.wedpr.hive.udf.config.ConfigLoader;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
import java.math.BigDecimal;
import javax.xml.bind.DatatypeConverter;
import org.apache.hadoop.hive.ql.exec.Description;
import org.apache.hadoop.hive.ql.exec.UDF;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author caryliao
 * @date 2023/11/28
 */
@Description(
        name = "ihc_enc",
        value = "迭代希尔密码加密",
        extended =
                "用法:ihc_enc(columnName, hexSk, iterRound, keyBits) \n"
                        + "示例:select ihc_enc(salary, '303132...', 10, 64) from t_user")
public class IhcEncryptionUDF extends UDF {
    private static final Logger logger = LoggerFactory.getLogger(IhcEncryptionUDF.class);

    private final NumberCodec codec = new NumberCodecImpl();

    public String evaluate(final String plainValue) {
        KeyConfig config = ConfigLoader.config.getKeyConfig();
        return evaluate(config.getIhcMode(), config.getIhcPrivateKey(), plainValue);
    }
    /**
     * ihc_enc(columnName, hexSk, iterRound, keyBits)
     *
     * @param plainValue
     * @param hexSk
     * @return
     */
    public String evaluate(int mode, final String hexSk, final String plainValue) {
        try {
            if (plainValue == null) {
                return null;
            }
            FloatingPointNumber plainValueFp = codec.encode(new BigDecimal(plainValue));
            byte[] cipher =
                    NativeFloatingIhc.encrypt(
                            mode,
                            DatatypeConverter.parseHexBinary(hexSk),
                            plainValueFp.getSignificantBytes(),
                            plainValueFp.getExponent());
            return DatatypeConverter.printHexBinary(cipher);
        } catch (Exception e) {
            logger.error("ihc加密失败 ", e);
            throw new DecryptionException("ihc加密失败", e);
        }
    }
}
