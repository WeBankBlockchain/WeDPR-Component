package com.webank.wedpr.pir.crypto.service.impl;

import com.webank.wedpr.pir.crypto.entity.PirTable;
import com.webank.wedpr.pir.crypto.service.PirTableService;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;
import org.springframework.stereotype.Service;

/**
 * @author zachma
 * @date 2024/8/19
 */
@Service
public class PirTableServiceImpl implements PirTableService {

    @PersistenceContext private EntityManager entityManager;

    @Override
    public List idFilterTable(String datasetId, String filter, String[] params) {
        String param = processParams(params);
        String sqlFormat = "SELECT t.id as t_id, %s FROM %s t WHERE t.id LIKE CONCAT(%s, '%%')";
        String sql = String.format(sqlFormat, param, datasetId, filter);
        Query query = entityManager.createNativeQuery(sql);
        return query.getResultList();
    }

    @Override
    public List idObfuscationTable(String datasetId, String searchId, String[] params) {
        String param = processParams(params);
        String sqlFormat = "SELECT t.id as t_id, %s FROM %s t WHERE t.id = %s";
        String sql = String.format(sqlFormat, param, datasetId, searchId);
        Query query = entityManager.createNativeQuery(sql);
        return query.getResultList();
    }

    public List<PirTable> objectsToPirTableList(List values) {
        List<PirTable> result = new LinkedList<>();
        for (int i = 0; i < values.size(); i++) {
            Object[] temp = (Object[]) values.get(i);
            PirTable pirTable = new PirTable();
            pirTable.setId(i + 1);
            pirTable.setPirKey(String.valueOf(temp[0]));
            Object[] objects = Arrays.copyOfRange(temp, 1, temp.length);
            pirTable.setPirValue(Arrays.toString(objects));
            result.add(pirTable);
        }
        return result;
    }

    private String processParams(String[] params) {
        String[] temp = Arrays.stream(params).map(s -> "t." + s).toArray(String[]::new);
        return String.join(",", temp);
    }
}
