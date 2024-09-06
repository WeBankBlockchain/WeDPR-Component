package com.wedpr.pir.sdk.entity.request;

import com.wedpr.pir.sdk.entity.body.PirDataBody;
import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import java.math.BigInteger;
import java.util.List;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * @author zachma
 * @date 2024/8/18
 */
@EqualsAndHashCode(callSuper = true)
@Data
public class ServerJobRequest extends PirBaseRequest {
    BigInteger x;
    BigInteger y;
    List<PirDataBody> dataBodyList;
    ServiceConfigBody serviceConfigBody;
}
