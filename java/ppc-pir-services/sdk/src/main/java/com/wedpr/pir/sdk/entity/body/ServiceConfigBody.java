package com.wedpr.pir.sdk.entity.body;

import lombok.Data;

/**
 * @author zachma
 * @date 2024/9/3
 */
@Data
public class ServiceConfigBody {
    private String datasetId;
    private String[] exists;
    private String[] values;
}
