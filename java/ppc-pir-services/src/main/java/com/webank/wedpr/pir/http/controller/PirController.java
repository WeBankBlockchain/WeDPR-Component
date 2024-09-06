package com.webank.wedpr.pir.http.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.webank.wedpr.pir.http.common.Constant;
import com.webank.wedpr.pir.http.service.PirAppService;
import com.webank.wedpr.pir.http.service.PirAuthService;
import com.webank.wedpr.pir.http.service.PirDataService;
import com.wedpr.pir.sdk.PirClient;
import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import com.wedpr.pir.sdk.entity.param.PirJobParam;
import com.wedpr.pir.sdk.entity.request.ClientAuthRequest;
import com.wedpr.pir.sdk.entity.request.ClientDirectRequest;
import com.wedpr.pir.sdk.entity.request.ServerJobRequest;
import com.wedpr.pir.sdk.entity.response.ClientJobResponse;
import com.wedpr.pir.sdk.entity.response.PirResultResponse;
import com.wedpr.pir.sdk.entity.response.ServerOTResponse;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import lombok.extern.slf4j.Slf4j;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

/**
 * @author zachma
 * @date 2024/8/18
 */
@RestController
@Slf4j
public class PirController {
    private static final Logger logger = LoggerFactory.getLogger(PirController.class);

    @Autowired private ObjectMapper objectMapper;
    @Autowired private PirAppService pirAppService;
    @Autowired private PirAuthService pirAuthService;
    @Autowired private PirDataService pirDataService;

    @PostMapping(Constant.PIR_API_PREFIX + "/auth")
    public ClientJobResponse pirAuthController(@RequestBody ClientAuthRequest clientAuthRequest) {
        try {
            logger.info(
                    "ClientAuthRequest: clientAuthRequest: {}.",
                    objectMapper.writeValueAsString(clientAuthRequest));
            // 1. 发起认证auth
            ServiceConfigBody serviceConfigBody =
                    pirAuthService.checkServiceAuth(clientAuthRequest);
            return new ClientJobResponse(WedprStatusEnum.SUCCESS, serviceConfigBody);
        } catch (Exception e) {
            logger.warn(
                    "匿踪任务认证失败, serviceId: {}, response: {}",
                    clientAuthRequest.getServiceId(),
                    WedprStatusEnum.SYSTEM_EXCEPTION.getMessage());
            return new ClientJobResponse(WedprStatusEnum.SYSTEM_EXCEPTION, null);
        }
    }

    @PostMapping(Constant.PIR_API_PREFIX + "/client")
    public PirResultResponse pirClientController(
            @RequestBody ClientDirectRequest clientDirectRequest) {
        try {
            logger.info(
                    "ClientDirectRequest: clientDirectRequest: {}.",
                    objectMapper.writeValueAsString(clientDirectRequest));
            PirJobParam pirJobParam = new PirJobParam();
            pirJobParam.setPirInvokeType(ParamEnum.JobMode.DirectorMode.getValue());
            pirJobParam.setJobType(ParamEnum.JobType.SearchValue.getValue());
            pirJobParam.setJobAlgorithmType(clientDirectRequest.getAlgorithmType());

            // todo:对接网关
            pirJobParam.setGatewayUrl("localhost:5831");

            pirJobParam.setSearchIdList(clientDirectRequest.getSearchIds());
            pirJobParam.setServiceConfigBody(clientDirectRequest.getServiceConfigBody());

            PirClient pirClient = new PirClient(pirJobParam);
            return pirClient.executePirJob();
        } catch (Exception e) {
            logger.warn("匿踪任务失败, response: {}", WedprStatusEnum.SYSTEM_EXCEPTION.getMessage());
            return null;
        }
    }

    @PostMapping(Constant.PIR_API_PREFIX + "/server")
    public ClientJobResponse pirServerController(@RequestBody ServerJobRequest serverJobRequest) {
        try {
            pirDataService.processPirDataset(serverJobRequest.getServiceConfigBody());
            logger.info(
                    "WedprServerjob: serverJobRequest: {}.",
                    objectMapper.writeValueAsString(serverJobRequest));
            // 1. 根据请求，筛选数据，加密密钥，返回筛选结果及AES消息密文
            ServerOTResponse serverOTResponse = pirAppService.providerOtCipher(serverJobRequest);
            return new ClientJobResponse(WedprStatusEnum.SUCCESS, serverOTResponse);
        } catch (Exception e) {
            e.printStackTrace();
            logger.warn(
                    "匿踪任务失败, serviceId: {}, response: {}",
                    serverJobRequest.getServiceId(),
                    WedprStatusEnum.SYSTEM_EXCEPTION.getMessage());
            return new ClientJobResponse(WedprStatusEnum.SYSTEM_EXCEPTION, e.getMessage());
        }
    }
}
