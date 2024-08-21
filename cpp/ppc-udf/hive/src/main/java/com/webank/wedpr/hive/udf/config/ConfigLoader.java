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

import com.moandjiezana.toml.Toml;
import com.webank.wedpr.hive.udf.config.model.ConfigProperty;
import com.webank.wedpr.hive.udf.exceptions.ConfigException;
import java.io.InputStream;
import org.apache.log4j.Logger;

public class ConfigLoader {
    private static final Logger logger = Logger.getLogger(ConfigLoader.class.getName());
    private static final String DEFAULT_CONFIG_FILE = "config.toml";
    public static Config config;

    static {
        InputStream configStream =
                ConfigLoader.class.getClassLoader().getResourceAsStream(DEFAULT_CONFIG_FILE);
        if (configStream == null) {
            throw new ConfigException(
                    "The config file " + DEFAULT_CONFIG_FILE + " doesn't exists!");
        }
        config = load(configStream);
    }

    private static Config load(InputStream configStream) throws ConfigException {
        try {
            ConfigProperty property = new Toml().read(configStream).to(ConfigProperty.class);
            return new Config(property);
        } catch (Exception e) {
            logger.error("load config error, e: " + e.getMessage());
            throw new ConfigException("load config error!", e);
        }
    }
}
