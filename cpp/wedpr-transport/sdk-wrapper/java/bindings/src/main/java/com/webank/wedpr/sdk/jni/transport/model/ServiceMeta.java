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

import java.util.ArrayList;
import java.util.List;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.ToString;

@Data
@NoArgsConstructor
@ToString
public class ServiceMeta {
    List<EntryPointInfo> serviceInfos = new ArrayList<>();

    public void addServiceInfo(EntryPointInfo entryPointInfo) {
        serviceInfos.add(entryPointInfo);
    }

    public void setServiceInfos(List<EntryPointInfo> serviceInfos) {
        if (serviceInfos == null) {
            return;
        }
        this.serviceInfos = serviceInfos;
    }
}
