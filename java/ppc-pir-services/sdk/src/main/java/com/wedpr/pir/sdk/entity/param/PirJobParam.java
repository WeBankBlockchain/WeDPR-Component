package com.wedpr.pir.sdk.entity.param;

import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import com.wedpr.pir.sdk.entity.request.PirBaseRequest;
import com.wedpr.pir.sdk.enums.ParamEnum;
import com.wedpr.pir.sdk.exception.WedprException;
import com.wedpr.pir.sdk.exception.WedprStatusEnum;
import com.wedpr.pir.sdk.helper.BasicTypeHelper;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * @author zachma
 * @date 2024/8/19
 */
@Data
@EqualsAndHashCode(callSuper = true)
public class PirJobParam extends PirBaseRequest {

    private Integer filterLength = 4;

    private String gatewayUrl;

    private List<String> searchIdList;

    private ServiceConfigBody serviceConfigBody;

    public void check() throws WedprException {
        if (BasicTypeHelper.isStringEmpty(gatewayUrl)) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "访问gatewayUrl不能为空");
        }

        if (getPirInvokeType().equals(ParamEnum.JobMode.SDKMode.getValue())
                && BasicTypeHelper.isStringEmpty(getServiceId())) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "serviceId 不能为空");
        }

        if (getPirInvokeType().equals(ParamEnum.JobMode.SDKMode.getValue())
                && BasicTypeHelper.isStringEmpty(getAccessKeyId())) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "accessKeyId 不能为空");
        }

        if (Arrays.stream(ParamEnum.JobType.values())
                .noneMatch(enumValue -> enumValue.getValue().equals(getJobType()))) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "jobType输入错误");
        }

        if (Arrays.stream(ParamEnum.AlgorithmType.values())
                .noneMatch(enumValue -> enumValue.getValue().equals(getJobAlgorithmType()))) {
            throw new WedprException(
                    WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "jobAlgorithmType输入错误");
        }

        if (Objects.isNull(searchIdList) || searchIdList.size() == 0) {
            throw new WedprException(WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "searchId列表不能为空");
        }

        if (getPirInvokeType().equals(ParamEnum.JobMode.DirectorMode.getValue())
                && Objects.isNull(serviceConfigBody)) {
            throw new WedprException(
                    WedprStatusEnum.CLIENT_PARAM_EXCEPTION, "向导模式下serviceConfigBody不能为空");
        }
    }
}
