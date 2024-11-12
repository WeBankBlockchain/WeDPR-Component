/*
 * Copyright 2017-2025  [webank-wedpr]
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */

package com.webank.wedpr.sdk.jni.transport.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.webank.wedpr.sdk.jni.common.Common;
import com.webank.wedpr.sdk.jni.common.Constant;
import java.util.ArrayList;
import java.util.List;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.ToString;
import org.apache.commons.lang3.StringUtils;

@Data
@NoArgsConstructor
@ToString
@JsonIgnoreProperties(ignoreUnknown = true)
public class ServiceMeta {
    @Data
    @NoArgsConstructor
    @ToString
    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class EntryPointMeta {
        private String serviceName;
        private String entryPoint;

        public EntryPointMeta(String serviceName, String entryPoint) {
            this.serviceName = serviceName;
            this.entryPoint = entryPoint;
        }

        public String getUrl(String uriPath) {
            if (StringUtils.isBlank(uriPath)) {
                return Common.getUrl(entryPoint);
            }
            if (uriPath.startsWith(Constant.URI_SPLITER)) {
                return Common.getUrl(entryPoint + uriPath);
            }
            return Common.getUrl(entryPoint + Constant.URI_SPLITER + uriPath);
        }
    }
    // Note: here can't been changed
    private List<EntryPointMeta> serviceInfos = new ArrayList<>();

    public void setServiceInfos(List<EntryPointMeta> serviceInfos) {
        if (serviceInfos == null) {
            return;
        }
        this.serviceInfos = serviceInfos;
    }

    public void addEntryPoint(EntryPointMeta entryPointMeta) {
        this.serviceInfos.add(entryPointMeta);
    }
}
