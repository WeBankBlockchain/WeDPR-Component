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
package com.webank.wedpr.hive.udf.config.model;

import com.webank.wedpr.hive.udf.exceptions.ConfigException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
import java.util.Map;
import org.apache.log4j.Logger;

public class KeyConfig {
    private static final Logger logger = Logger.getLogger(KeyConfig.class.getName());

    // AES key
    private static final String AES_PRIVATE_KEY_PROPERTY = "aes_sk";
    private String aesPrivateKey;
    // AES IV
    private static final String AES_IV_PROPERTY = "aes_iv";
    private String aesIv;
    // ore key
    private static final String ORE_PRIVATE_KEY_PROPERTY = "ore_sk";
    private String orePrivateKey;

    // ihc key
    private static final String IHC_PRIVATE_KEY_PROPERTY = "ihc_sk";
    private String ihcPrivateKey;

    // paillier key
    private static final String PAILLIER_PRIVATE_KEY_PROPERTY = "paillier_sk";
    private static final String PAILLIER_PUBLIC_KEY_PROPERTY = "paillier_pk";
    private String paillierSk;
    private String paillierPk;

    private final int ihcMode = NativeFloatingIhc.mode.IHC_128.ordinal();

    public KeyConfig(ConfigProperty configProperty) throws ConfigException {
        Map<String, Object> keyConfig = configProperty.getKeys();
        /// load the aes private key
        aesPrivateKey = ConfigProperty.getValue(keyConfig, AES_PRIVATE_KEY_PROPERTY, null);
        if (aesPrivateKey == null) {
            throw new ConfigException("Must set the aes private key!");
        }
        /// load the aesIv
        aesIv = ConfigProperty.getValue(keyConfig, AES_IV_PROPERTY, null);
        if (aesIv == null) {
            throw new ConfigException("Must set the aes iv!");
        }
        /// load the orePrivateKey
        orePrivateKey = ConfigProperty.getValue(keyConfig, ORE_PRIVATE_KEY_PROPERTY, null);
        if (orePrivateKey == null) {
            throw new ConfigException("Must set the ore private key!");
        }
        /// load the ihcPrivateKey
        ihcPrivateKey = ConfigProperty.getValue(keyConfig, IHC_PRIVATE_KEY_PROPERTY, null);
        if (ihcPrivateKey == null) {
            throw new ConfigException("Must set the ihc private key");
        }
        /// load the paillier keys
        paillierPk = ConfigProperty.getValue(keyConfig, PAILLIER_PUBLIC_KEY_PROPERTY, null);
        if (paillierPk == null) {
            throw new ConfigException("Must set the paillier public key!");
        }
        paillierSk = ConfigProperty.getValue(keyConfig, PAILLIER_PRIVATE_KEY_PROPERTY, null);
        if (paillierSk == null) {
            throw new ConfigException("Must set the paillier private key");
        }
    }

    public String getAesPrivateKey() {
        return aesPrivateKey;
    }

    public void setAesPrivateKey(String aesPrivateKey) {
        this.aesPrivateKey = aesPrivateKey;
    }

    public String getOrePrivateKey() {
        return orePrivateKey;
    }

    public void setOrePrivateKey(String orePrivateKey) {
        this.orePrivateKey = orePrivateKey;
    }

    public String getIhcPrivateKey() {
        return ihcPrivateKey;
    }

    public void setIhcPrivateKey(String ihcPrivateKey) {
        this.ihcPrivateKey = ihcPrivateKey;
    }

    public String getAesIv() {
        return aesIv;
    }

    public void setAesIv(String aesIv) {
        this.aesIv = aesIv;
    }

    public String getPaillierSk() {
        return paillierSk;
    }

    public void setPaillierSk(String paillierSk) {
        this.paillierSk = paillierSk;
    }

    public String getPaillierPk() {
        return paillierPk;
    }

    public void setPaillierPk(String paillierPk) {
        this.paillierPk = paillierPk;
    }

    public int getIhcMode() {
        return this.ihcMode;
    }
}
