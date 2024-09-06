package com.webank.wedpr.pir.http.mapper;

import com.webank.wedpr.pir.crypto.entity.Dataset;
import com.webank.wedpr.pir.crypto.entity.WedprAuthTable;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Result;
import org.apache.ibatis.annotations.Results;
import org.apache.ibatis.annotations.Select;
import org.apache.ibatis.annotations.Update;

/**
 * @author zachma
 * @date 2024/9/3
 */
@Mapper
public interface WedprServiceAuthTableMapper {

    @Select(
            "SELECT service_id, access_key_id, expire_time FROM wedpr_service_auth_table WHERE service_id = #{serviceId} AND access_key_id = #{accessKeyId}")
    @Results({
        @Result(property = "serviceId", column = "service_id"),
        @Result(property = "accessKeyId", column = "access_key_id"),
        @Result(property = "expireTime", column = "expire_time")
    })
    WedprAuthTable searchWedprAuthTable(
            @Param("serviceId") String serviceId, @Param("accessKeyId") String accessKeyId);

    @Select("SELECT service_config FROM wedpr_published_service WHERE service_id = #{serviceId}")
    String searchWedprServicePublishTable(@Param("serviceId") String serviceId);

    @Select("SHOW TABLES")
    List<String> showAllTables();

    @Select(
            "SELECT dataset_id, dataset_fields, dataset_storage_path FROM wedpr_dataset WHERE dataset_id = #{datasetId}")
    @Results({
        @Result(property = "datasetId", column = "dataset_id"),
        @Result(property = "datasetFields", column = "dataset_fields"),
        @Result(property = "datasetStoragePath", column = "dataset_storage_path")
    })
    Dataset searchWedprDatasetId(@Param("datasetId") String datasetId);

    @Update(value = "${nativeSql}")
    void executeTableByNativeSql(@Param("nativeSql") String nativeSql);
}
