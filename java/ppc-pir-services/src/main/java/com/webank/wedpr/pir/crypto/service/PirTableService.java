package com.webank.wedpr.pir.crypto.service;

import com.webank.wedpr.pir.crypto.entity.PirTable;
import java.util.List;

/**
 * @author zachma
 * @date 2024/8/19
 */
public interface PirTableService {
    /** 根据id查询 */
    List idFilterTable(String datasetId, String filter, String[] param);

    List idObfuscationTable(String datasetId, String searchId, String[] params);

    List<PirTable> objectsToPirTableList(List params);
}
