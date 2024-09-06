package com.wedpr.pir.sdk;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import com.wedpr.pir.sdk.entity.body.SimpleEntity;
import com.wedpr.pir.sdk.entity.param.PirJobParam;
import com.wedpr.pir.sdk.entity.request.ClientAuthRequest;
import com.wedpr.pir.sdk.entity.request.ServerJobRequest;
import com.wedpr.pir.sdk.entity.response.ClientAuthResponse;
import com.wedpr.pir.sdk.entity.response.ClientOTResponse;
import com.wedpr.pir.sdk.entity.response.PirResultResponse;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import com.wedpr.pir.sdk.gateway.GatewayClient;
import com.wedpr.pir.sdk.helper.Constant;
import com.wedpr.pir.sdk.service.PirJobService;
import java.util.Objects;

/**
 * @author zachma
 * @date 2024/8/19
 */
public class PirClient {
    private final PirJobService pirJobService;

    public PirClient(PirJobParam pirJobParam) throws WedprException {
        pirJobParam.check();
        this.pirJobService = new PirJobService(pirJobParam);
    }

    public PirResultResponse executePirJob() throws WedprException {
        try {
            ObjectMapper objectMapper = new ObjectMapper();
            // 认证
            PirJobParam pirJobParam = pirJobService.getPirJobParam();
            ServiceConfigBody serviceConfigBody = pirJobParam.getServiceConfigBody();
            Integer invokeType = pirJobParam.getPirInvokeType();
            if (invokeType.equals(ParamEnum.JobMode.SDKMode.getValue())) {
                ClientAuthRequest clientAuthRequest = new ClientAuthRequest();
                clientAuthRequest.setAccessKeyId(pirJobParam.getAccessKeyId());
                clientAuthRequest.setServiceId(pirJobParam.getServiceId());
                String authUrl = pirJobParam.getGatewayUrl() + Constant.PERMISSION;
                String body = objectMapper.writeValueAsString(clientAuthRequest);
                ClientAuthResponse authResult =
                        GatewayClient.sendPostRequestWithRetry(
                                authUrl, body, ClientAuthResponse.class);
                if (!authResult.getCode().equals(WedprStatusEnum.SUCCESS.getCode())) {
                    throw new WedprException(
                            WedprStatusEnum.CALL_PSI_ERROR, "认证失败：" + authResult.getMessage());
                }
                pirJobParam.setServiceConfigBody(authResult.getData());
            }

            // 执行任务
            ClientOTResponse otParamResponse = pirJobService.requesterOtCipher();
            ServerJobRequest serverJobRequest =
                    generatePirJobRequestFromJobAndOTParam(pirJobParam, otParamResponse);

            String pirUrl = pirJobParam.getGatewayUrl() + Constant.SERVER;
            String body = objectMapper.writeValueAsString(serverJobRequest);
            SimpleEntity otResult =
                    GatewayClient.sendPostRequestWithRetry(pirUrl, body, SimpleEntity.class);
            if (Objects.isNull(otResult)
                    || !otResult.getCode().equals(WedprStatusEnum.SUCCESS.getCode())) {
                throw new WedprException(WedprStatusEnum.TASK_EXECUTE_ERROR);
            }
            return pirJobService.requesterOtRecover(otParamResponse, pirJobParam, otResult);
        } catch (Exception e) {
            throw new WedprException(WedprStatusEnum.CALL_PSI_ERROR, e.getMessage());
        }
    }

    /** 根据 ClientOTResponse 和 PirJobParam 构建PirJobRequest */
    private ServerJobRequest generatePirJobRequestFromJobAndOTParam(
            PirJobParam pirJobParam, ClientOTResponse otParamResponse) {
        ServerJobRequest serverJobRequest = new ServerJobRequest();
        serverJobRequest.setServiceId(pirJobParam.getServiceId());
        serverJobRequest.setServiceConfigBody(pirJobParam.getServiceConfigBody());
        serverJobRequest.setAccessKeyId(pirJobParam.getAccessKeyId());
        serverJobRequest.setJobAlgorithmType(pirJobParam.getJobAlgorithmType());
        serverJobRequest.setX(otParamResponse.getX());
        serverJobRequest.setY(otParamResponse.getY());
        serverJobRequest.setDataBodyList(otParamResponse.getDataBodyList());
        for (int i = 0; i < serverJobRequest.getDataBodyList().size(); i++) {
            serverJobRequest.getDataBodyList().get(i).setIdIndex(0);
        }
        return serverJobRequest;
    }
}
