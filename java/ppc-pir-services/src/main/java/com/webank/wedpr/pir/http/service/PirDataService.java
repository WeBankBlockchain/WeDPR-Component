package com.webank.wedpr.pir.http.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.webank.wedpr.pir.crypto.entity.Dataset;
import com.webank.wedpr.pir.crypto.entity.DatasetStoragePath;
import com.webank.wedpr.pir.http.common.CSVFileParser;
import com.webank.wedpr.pir.http.mapper.WedprServiceAuthTableMapper;
import com.wedpr.pir.sdk.entity.body.ServiceConfigBody;
import com.wedpr.pir.sdk.helper.Constant;
import java.util.Arrays;
import java.util.List;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

/**
 * @author zachma
 * @date 2024/9/4
 */
@Service
public class PirDataService {

    @Autowired private WedprServiceAuthTableMapper wedprServiceAuthTableMapper;

    @Autowired private FileSystem fileSystem;

    @PersistenceContext private EntityManager entityManager;

    public void processPirDataset(ServiceConfigBody serviceConfigBody) throws Exception {
        // 数据预处理
        List<String> allTables = wedprServiceAuthTableMapper.showAllTables();
        String tableId = Constant.datasetId2tableId(serviceConfigBody.getDatasetId());
        if (allTables.contains(tableId)) {
            return;
        }
        Dataset dataset =
                wedprServiceAuthTableMapper.searchWedprDatasetId(serviceConfigBody.getDatasetId());
        ObjectMapper objectMapper = new ObjectMapper();
        DatasetStoragePath datasetStoragePath =
                objectMapper.readValue(dataset.getDatasetStoragePath(), DatasetStoragePath.class);

        String csvFilePath = "./" + serviceConfigBody.getDatasetId();
        fileSystem.copyToLocalFile(
                new Path(datasetStoragePath.getFilePath()), new Path(csvFilePath));

        String[] datasetFields =
                Arrays.stream(dataset.getDatasetFields().trim().split(","))
                        .map(String::trim)
                        .toArray(String[]::new);
        processCsv2Db(serviceConfigBody.getDatasetId(), datasetFields, csvFilePath);
    }

    private void processCsv2Db(String datasetId, String[] datasetFields, String csvFilePath)
            throws Exception {
        List<List<String>> sqlValues = CSVFileParser.processCsv2SqlMap(datasetFields, csvFilePath);
        if (sqlValues.size() == 0) {
            throw new Exception("数据集为空,请退回");
        }

        String tableId = Constant.datasetId2tableId(datasetId);
        String createSqlFormat = "CREATE TABLE %s ( %s , PRIMARY KEY (`id`) USING BTREE )";

        String[] fieldsWithType = new String[datasetFields.length];
        for (int i = 0; i < datasetFields.length; i++) {
            if ("id".equalsIgnoreCase(datasetFields[i])) {
                fieldsWithType[i] = datasetFields[i] + " VARCHAR(64)";
            } else {
                fieldsWithType[i] = datasetFields[i] + " TEXT";
            }
        }
        String sql = String.format(createSqlFormat, tableId, String.join(",", fieldsWithType));
        wedprServiceAuthTableMapper.executeTableByNativeSql(sql);

        StringBuilder sb = new StringBuilder();
        for (List<String> values : sqlValues) {
            sb.append("(").append(String.join(",", values)).append("), ");
        }
        String insertValues = sb.toString();
        insertValues = insertValues.substring(0, insertValues.length() - 2);

        String insertSqlFormat = "INSERT INTO %s (%s) VALUES %s ";
        sql =
                String.format(
                        insertSqlFormat, tableId, String.join(",", datasetFields), insertValues);
        wedprServiceAuthTableMapper.executeTableByNativeSql(sql);
    }
}
