package com.webank.wedpr.pir.http.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.webank.wedpr.pir.crypto.entity.WedprAuthTable;
import com.webank.wedpr.pir.http.mapper.WedprServiceAuthTableMapper;
import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import com.wedpr.pir.sdk.entity.request.ClientAuthRequest;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import java.time.LocalDateTime;
import java.util.Objects;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

/**
 * @author zachma
 * @date 2024/9/3
 */
@Service
public class PirAuthService {

    @Autowired private WedprServiceAuthTableMapper wedprServiceAuthTableMapper;

    public ServiceConfigBody checkServiceAuth(ClientAuthRequest clientAuthRequest)
            throws WedprException {
        try {
            WedprAuthTable wedprAuthTable =
                    wedprServiceAuthTableMapper.searchWedprAuthTable(
                            clientAuthRequest.getServiceId(), clientAuthRequest.getAccessKeyId());
            if (Objects.isNull(wedprAuthTable)) {
                throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "用户未申请认证");
            }

            if (LocalDateTime.now().isAfter(wedprAuthTable.getExpireTime())) {
                throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "认证已经过期");
            }

            String serviceConfigStr =
                    wedprServiceAuthTableMapper.searchWedprServicePublishTable(
                            clientAuthRequest.getServiceId());
            ObjectMapper objectMapper = new ObjectMapper();
            ServiceConfigBody serviceConfigBody =
                    objectMapper.readValue(serviceConfigStr, ServiceConfigBody.class);

            return serviceConfigBody;
        } catch (Exception e) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, e.getMessage());
        }
    }
}
