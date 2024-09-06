package com.wedpr.pir.sdk.entity.request;

import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import java.util.List;
import lombok.Data;

/**
 * @author zachma
 * @date 2024/9/5
 */
@Data
public class ClientDirectRequest {
    private String algorithmType;
    private List<String> searchIds;
    private ServiceConfigBody serviceConfigBody;
}
