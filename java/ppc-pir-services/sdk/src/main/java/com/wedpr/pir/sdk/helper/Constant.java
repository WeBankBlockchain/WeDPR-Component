package com.wedpr.pir.sdk.helper;
/**
 * @author zachma
 * @date 2024/9/3
 */
public class Constant {

    public static final String SERVER = "/api/pir/v3/server";

    public static final String PERMISSION = "/api/pir/v3/auth";

    public static String PIR_TEMP_TABLE_PREFIX = "wedpr_pir_";

    public static String datasetId2tableId(String datasetId) {
        return PIR_TEMP_TABLE_PREFIX + datasetId.substring(2);
    }
}
