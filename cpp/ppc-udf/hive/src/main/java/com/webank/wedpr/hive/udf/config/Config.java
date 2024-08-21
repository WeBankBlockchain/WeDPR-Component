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
package com.webank.wedpr.hive.udf.config;

import com.webank.wedpr.hive.udf.config.model.ConfigProperty;
import com.webank.wedpr.hive.udf.config.model.KeyConfig;
import java.util.Objects;
import org.apache.log4j.Logger;

public class Config {
    private static final Logger logger = Logger.getLogger(Config.class.getName());

    private KeyConfig keyConfig;

    public Config(ConfigProperty configProperty) {
        this.keyConfig = new KeyConfig(configProperty);
    }

    public KeyConfig getKeyConfig() {
        return keyConfig;
    }

    public void setKeyConfig(KeyConfig keyConfig) {
        this.keyConfig = keyConfig;
    }

    @Override
    public String toString() {
        return "Config{" + "keyConfig=" + keyConfig + '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Config that = (Config) o;
        return Objects.equals(keyConfig, that.keyConfig);
    }

    @Override
    public int hashCode() {
        return Objects.hash(keyConfig);
    }
}
