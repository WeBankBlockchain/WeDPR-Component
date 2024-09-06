package com.wedpr.pir.sdk.entity.response;

import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import lombok.Data;

/**
 * @author zachma
 * @date 2024/9/3
 */
@Data
public class ClientAuthResponse {
    String code;
    String message;
    ServiceConfigBody data;
}
