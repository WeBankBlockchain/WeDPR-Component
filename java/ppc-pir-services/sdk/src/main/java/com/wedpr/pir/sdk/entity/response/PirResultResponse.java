package com.wedpr.pir.sdk.entity.response;

import lombok.Data;

/**
 * @author zachma
 * @date 2024/8/21
 */
@Data
public class PirResultResponse {
    String jobType;
    // List<PirResultBody> detail;
    ClientDecryptResponse detail;
}
