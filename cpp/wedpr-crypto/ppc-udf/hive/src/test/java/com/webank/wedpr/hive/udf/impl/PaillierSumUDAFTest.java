package com.webank.wedpr.hive.udf.impl;

import java.math.BigDecimal;

import com.webank.wedpr.hive.udf.impl.paillier.PaillierDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.paillier.PaillierEncryptionUDF;
import com.webank.wedpr.hive.udf.impl.paillier.PaillierSumUDAF;
import org.apache.hadoop.hive.ql.metadata.HiveException;
import org.apache.hadoop.hive.ql.udf.generic.GenericUDAFEvaluator;
import org.apache.hadoop.hive.ql.udf.generic.GenericUDAFParameterInfo;
import org.apache.hadoop.hive.ql.udf.generic.SimpleGenericUDAFParameterInfo;
import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.JavaConstantStringObjectInspector;
import org.apache.hadoop.io.Text;
import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author caryliao
 *  2023/11/9
 */

public class PaillierSumUDAFTest {
    private static final Logger logger = LoggerFactory.getLogger(PaillierSumUDAFTest.class);

    @Test
    public void testPaillierSumUDAFEvaluate() throws HiveException {
        PaillierDecryptionUDF paillierDecUDF = new PaillierDecryptionUDF();
        PaillierEncryptionUDF encryptionUDF = new PaillierEncryptionUDF();
        String plain = "1232324234.34353543";
        Text cipher = encryptionUDF.evaluate(new Text(plain));

        PaillierSumUDAF paillierSumUDAF = new PaillierSumUDAF();
        JavaConstantStringObjectInspector stringObjectInspector1 = new JavaConstantStringObjectInspector(cipher.toString());
        ObjectInspector[] params1 = {stringObjectInspector1};
        ObjectInspector[] params2 = {stringObjectInspector1};
        GenericUDAFParameterInfo parameterInfo = new SimpleGenericUDAFParameterInfo(params1, false, false, false);
        GenericUDAFEvaluator evaluator = paillierSumUDAF.getEvaluator(parameterInfo);
        evaluator.init(GenericUDAFEvaluator.Mode.PARTIAL1, params1);

        GenericUDAFEvaluator.AggregationBuffer agg = evaluator.getNewAggregationBuffer();
        String[] iterateParams1 = {cipher.toString()};
        evaluator.iterate(agg, iterateParams1);
        evaluator.init(GenericUDAFEvaluator.Mode.PARTIAL1, params2);
        String[] iterateParams2 = {cipher.toString()};
        evaluator.iterate(agg, iterateParams2);
        Text result = (Text) evaluator.terminate(agg);
        System.out.println("result:" + result.toString());

        String decryptValueStr = paillierDecUDF.evaluate(new Text(result)).toString();
        System.out.println("###### decryptValueStr: " + decryptValueStr);
        BigDecimal decryptValue = new BigDecimal(decryptValueStr);
        BigDecimal m = new BigDecimal(plain);
        BigDecimal expectedV = m.add(m);
        Assert.assertTrue(decryptValue.compareTo(expectedV) == 0);
    }

}

