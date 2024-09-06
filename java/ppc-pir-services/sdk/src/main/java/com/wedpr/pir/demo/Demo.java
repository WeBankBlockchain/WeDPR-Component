package com.wedpr.pir.demo;

import com.wedpr.pir.sdk.PirClient;
import com.wedpr.pir.sdk.entity.param.PirJobParam;
import com.wedpr.pir.sdk.entity.response.PirResultResponse;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import java.util.Arrays;

/**
 * @author zachma
 * @date 2024/8/22
 */
public class Demo {
    public static void main(String[] args) throws WedprException {
        testIdObfuscation();
        testIdFilter();
    }

    private static void testIdFilter() throws WedprException {
        PirJobParam pirJobParam = new PirJobParam();
        pirJobParam.setServiceId("s-123456789");
        pirJobParam.setAccessKeyId("a-123456789");
        pirJobParam.setJobType(ParamEnum.JobType.SearchValue.getValue());
        pirJobParam.setJobAlgorithmType(ParamEnum.AlgorithmType.idFilter.getValue());
        pirJobParam.setGatewayUrl("localhost:5831");
        pirJobParam.setSearchIdList(Arrays.asList("1", "2", "3"));

        PirClient pirClient = new PirClient(pirJobParam);
        PirResultResponse pirResultResponse = pirClient.executePirJob();
        System.out.println(pirResultResponse);
    }

    private static void testIdObfuscation() throws WedprException {
        PirJobParam pirJobParam = new PirJobParam();
        pirJobParam.setServiceId("s-123456789");
        pirJobParam.setAccessKeyId("a-123456789");
        pirJobParam.setJobType(ParamEnum.JobType.SearchValue.getValue());
        pirJobParam.setJobAlgorithmType(ParamEnum.AlgorithmType.idObfuscation.getValue());
        pirJobParam.setGatewayUrl("localhost:5831");
        pirJobParam.setSearchIdList(Arrays.asList("3", "2", "1"));

        PirClient pirClient = new PirClient(pirJobParam);
        PirResultResponse pirResultResponse = pirClient.executePirJob();
        System.out.println(pirResultResponse);
    }
}
