package com.wedpr.pir.sdk.entity.request;

import com.wedpr.pir.sdk.enums.ParamEnum;
import lombok.Data;

@Data
public class PirBaseRequest {
    // 任务ID
    // 任务类型(0: 查询存在性, 1: 查询对应值)
    String serviceId;

    String accessKeyId;

    String jobType;
    // 数据方机构id
    // 匿踪算法类型(0: hash披露算法, 1: hash混淆算法)
    String jobAlgorithmType;
    // 查询范围
    Integer obfuscationOrder = 9;

    Integer pirInvokeType = ParamEnum.JobMode.SDKMode.getValue();
}
