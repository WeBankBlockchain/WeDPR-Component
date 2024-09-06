package com.wedpr.pir.sdk.entity.request;

import com.wedpr.pir.sdk.entity.body.PirDataBody;
import java.math.BigInteger;
import java.util.List;
import lombok.Data;

/**
 * @author zachma
 * @date 2024/8/18
 */
@Data
public class ServerOTRequest {
    String jobType;
    String datasetId;
    String[] params;
    BigInteger x;
    BigInteger y;
    List<PirDataBody> dataBodyList;
}
