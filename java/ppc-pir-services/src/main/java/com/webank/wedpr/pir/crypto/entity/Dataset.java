package com.webank.wedpr.pir.crypto.entity;

import lombok.Data;

/**
 * @author zachma
 * @date 2024/9/4
 */
@Data
public class Dataset {
    private String datasetId;

    private String datasetFields;

    private String datasetStoragePath;
}
