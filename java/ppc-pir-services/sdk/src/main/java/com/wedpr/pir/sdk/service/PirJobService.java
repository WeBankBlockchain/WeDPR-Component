package com.wedpr.pir.sdk.service;

import com.wedpr.pir.sdk.entity.body.SimpleEntity;
import com.wedpr.pir.sdk.entity.param.PirJobParam;
import com.wedpr.pir.sdk.entity.request.ClientDecryptRequest;
import com.wedpr.pir.sdk.entity.request.ClientOTRequest;
import com.wedpr.pir.sdk.entity.response.ClientDecryptResponse;
import com.wedpr.pir.sdk.entity.response.ClientOTResponse;
import com.wedpr.pir.sdk.entity.response.PirResultResponse;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import lombok.Getter;

/**
 * @author zachma
 * @date 2024/8/20
 */
public class PirJobService {

    @Getter private final PirJobParam pirJobParam;
    private final ClientOTService clientOTService;
    private final ClientDecryptService clientDecryptService;

    public PirJobService(PirJobParam pirJobParam) {
        this.pirJobParam = pirJobParam;
        this.clientOTService = new ClientOTService();
        this.clientDecryptService = new ClientDecryptService();
    }

    public ClientOTResponse requesterOtCipher() throws WedprException {
        ClientOTRequest clientOTRequest = new ClientOTRequest();
        ClientOTResponse otParamResponse;
        clientOTRequest.setDataBodyList(pirJobParam.getSearchIdList());
        if (pirJobParam.getJobAlgorithmType().equals(ParamEnum.AlgorithmType.idFilter.getValue())) {
            clientOTRequest.setFilterLength(pirJobParam.getFilterLength());
            otParamResponse = clientOTService.runClientOTParam(clientOTRequest);
        } else if (pirJobParam
                .getJobAlgorithmType()
                .equals(ParamEnum.AlgorithmType.idObfuscation.getValue())) {
            otParamResponse = clientOTService.runClientOTCipher(clientOTRequest);
        } else {
            throw new WedprException(WedprStatusEnum.INVALID_INPUT_VALUE);
        }
        return otParamResponse;
    }

    public PirResultResponse requesterOtRecover(
            ClientOTResponse otParamResponse, PirJobParam clientJobRequest, SimpleEntity otResult) {
        ClientDecryptRequest clientDecryptRequest = new ClientDecryptRequest();
        clientDecryptRequest.setB(otParamResponse.getB());
        clientDecryptRequest.setDataBodyList(clientJobRequest.getSearchIdList());
        clientDecryptRequest.setServerResult(otResult.getData());

        ClientDecryptResponse clientDecryptResponse =
                clientDecryptService.runDecryptOTparam(clientDecryptRequest);

        PirResultResponse pirResultResponse = new PirResultResponse();
        pirResultResponse.setJobType(clientJobRequest.getJobType());
        pirResultResponse.setDetail(clientDecryptResponse);
        return pirResultResponse;
    }
}
