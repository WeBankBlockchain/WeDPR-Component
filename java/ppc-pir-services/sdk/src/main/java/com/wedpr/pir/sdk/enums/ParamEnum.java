package com.wedpr.pir.sdk.enums;

import lombok.AllArgsConstructor;
import lombok.Getter;

/**
 * @author zachma
 * @date 2024/8/18
 */
public class ParamEnum {
    @Getter
    @AllArgsConstructor
    public enum JobType {
        SearchValue("0"),
        SearchExist("1");
        private String value;
    }

    @Getter
    @AllArgsConstructor
    public enum AlgorithmType {
        idFilter("0"),
        idObfuscation("1");

        private String value;
    }

    @Getter
    @AllArgsConstructor
    public enum HttpType {
        HttpTimeout(180),
        HttpMaxRetries(1);
        private Integer value;
    }

    @Getter
    @AllArgsConstructor
    public enum JobMode {
        SDKMode(0),
        DirectorMode(1);
        private Integer value;
    }
}
