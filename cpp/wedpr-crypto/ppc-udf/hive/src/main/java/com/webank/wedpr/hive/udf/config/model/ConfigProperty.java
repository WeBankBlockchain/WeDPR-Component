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

import java.util.Map;
import java.util.Objects;
import org.apache.log4j.Logger;
import org.codehaus.jackson.annotate.JsonIgnoreProperties;

@JsonIgnoreProperties(ignoreUnknown = true)
public class ConfigProperty {
    private static final Logger logger = Logger.getLogger(ConfigProperty.class.getName());

    private Map<String, Object> keys;

    public Map<String, Object> getKeys() {
        return keys;
    }

    public void setKeys(Map<String, Object> keys) {
        this.keys = keys;
    }

    public static String getValue(Map<String, Object> config, String key, String defaultValue) {
        if (config == null || config.get(key) == null) {
            return defaultValue;
        }
        return (String) config.get(key);
    }

    @Override
    public String toString() {
        return "ConfigProperty{" + "keys=" + keys + '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ConfigProperty that = (ConfigProperty) o;
        return Objects.equals(keys, that.keys);
    }

    @Override
    public int hashCode() {
        return Objects.hash(keys);
    }
}
