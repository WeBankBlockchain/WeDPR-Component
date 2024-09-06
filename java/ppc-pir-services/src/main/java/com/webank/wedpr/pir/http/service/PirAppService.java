package com.webank.wedpr.pir.http.service;

import com.webank.wedpr.pir.crypto.service.PirOTService;
import com.wedpr.pir.sdk.entity.request.ServerJobRequest;
import com.wedpr.pir.sdk.entity.request.ServerOTRequest;
import com.wedpr.pir.sdk.entity.response.ServerOTResponse;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

/**
 * @author zachma
 * @date 2024/8/18
 */
@Service
public class PirAppService {

    @Autowired private PirOTService pirOTService;

    public ServerOTResponse providerOtCipher(ServerJobRequest serverJobRequest)
            throws WedprException {
        // 1. 根据请求，筛选数据，加密密钥，返回筛选结果及AES消息密文
        ServerOTRequest serverOTRequest = new ServerOTRequest();
        serverOTRequest.setJobType(serverJobRequest.getJobType());
        serverOTRequest.setDatasetId(serverJobRequest.getServiceConfigBody().getDatasetId());
        serverOTRequest.setX(serverJobRequest.getX());
        serverOTRequest.setY(serverJobRequest.getY());
        serverOTRequest.setDataBodyList(serverJobRequest.getDataBodyList());

        ServerOTResponse otResultResponse;
        if (serverJobRequest
                .getJobAlgorithmType()
                .equals(ParamEnum.AlgorithmType.idFilter.getValue())) {
            serverOTRequest.setParams(serverJobRequest.getServiceConfigBody().getValues());
            otResultResponse = pirOTService.runServerOTParam(serverOTRequest);
        } else if (serverJobRequest
                .getJobAlgorithmType()
                .equals(ParamEnum.AlgorithmType.idObfuscation.getValue())) {
            serverOTRequest.setParams(serverJobRequest.getServiceConfigBody().getExists());
            otResultResponse = pirOTService.runServerOTCipher(serverOTRequest);
        } else {
            throw new WedprException(WedprStatusEnum.INVALID_INPUT_VALUE);
        }
        return otResultResponse;
    }
}
