package com.webank.wedpr.hive.udf.impl;

import java.math.BigDecimal;

import com.webank.wedpr.hive.udf.impl.ihc.IhcDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcEncryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcSumUDAF;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
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
import com.webank.wedpr.hive.udf.impl.ihc.IhcDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcEncryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcSumUDAF;

import javax.xml.bind.DatatypeConverter;

/**
 * @author caryliao
 * @date 2023/11/30
 */

public class IhcSumUDAFTest {
    private static final Logger logger = LoggerFactory.getLogger(IhcSumUDAFTest.class);

    @Test
    public void testIhcSumUDAFEvaluate() throws HiveException, JniException
    {
             //testIhcSumUDAFEvaluateImpl(NativeFloatingIhc.mode.IHC_256.ordinal());
        testIhcSumUDAFEvaluateImpl(NativeFloatingIhc.mode.IHC_128.ordinal());
    }

    public void testIhcSumUDAFEvaluateImpl(int mode) throws HiveException, JniException {
        IhcEncryptionUDF ihcEncUDF = new IhcEncryptionUDF();
        String plainValue1 = "111.11";
        String plainValue2 = "222.22";
        String hexSk = DatatypeConverter.printHexBinary(NativeFloatingIhc.generateKey(mode));
        String cipherValue1 = ihcEncUDF.evaluate(mode, hexSk, plainValue1);
        String cipherValue2 = ihcEncUDF.evaluate(mode, hexSk, plainValue2);
        System.out.println("cipherValue1:" + cipherValue1);
        System.out.println("cipherValue2:" + cipherValue2);

        IhcSumUDAF ihcSumUDAF = new IhcSumUDAF();
        JavaConstantStringObjectInspector stringObjectInspector = new JavaConstantStringObjectInspector(cipherValue1);
        ObjectInspector[] params = {stringObjectInspector};
        GenericUDAFParameterInfo parameterInfo = new SimpleGenericUDAFParameterInfo(params, false, false, false);
        GenericUDAFEvaluator evaluator = ihcSumUDAF.getEvaluator(parameterInfo);
        evaluator.init(GenericUDAFEvaluator.Mode.PARTIAL1, params);
        GenericUDAFEvaluator.AggregationBuffer agg = evaluator.getNewAggregationBuffer();
        Object[] iterateParams1 = {cipherValue1};
        evaluator.iterate(agg, iterateParams1);
        evaluator.init(GenericUDAFEvaluator.Mode.PARTIAL1, params);
        Object[] iterateParams2 = {cipherValue2};
        evaluator.iterate(agg, iterateParams2);
        Text result = (Text) evaluator.terminate(agg);
        System.out.println("result:" + result.toString());

        long startTime = System.nanoTime();
        IhcDecryptionUDF ihcDecUDF = new IhcDecryptionUDF();
        String sumValueStr = ihcDecUDF.evaluate(mode, hexSk, result.toString());
        BigDecimal sumValue = new BigDecimal(sumValueStr);
        long endTime = System.nanoTime();
        System.out.println("paillierSumUDF cost time(ns):" + (endTime - startTime));
        System.out.println("sumValue:" + sumValue);
        Assert.assertTrue(new BigDecimal("333.33").compareTo(sumValue) == 0);;
    }

}

