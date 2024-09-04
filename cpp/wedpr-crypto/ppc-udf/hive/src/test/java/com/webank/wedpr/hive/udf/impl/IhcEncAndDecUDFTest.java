package com.webank.wedpr.hive.udf.impl;

import java.math.BigDecimal;
import javax.xml.bind.DatatypeConverter;

import com.webank.wedpr.hive.udf.impl.ihc.IhcDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcEncryptionUDF;
import org.apache.hadoop.io.Text;
import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.webank.wedpr.hive.udf.impl.ihc.IhcDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ihc.IhcEncryptionUDF;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;

/**
 * @author caryliao
 * @date 2023/11/29
 */

public class IhcEncAndDecUDFTest {
    private static final Logger logger = LoggerFactory.getLogger(IhcEncAndDecUDFTest.class);

    public void testEncAndDecEvaluateImpl(int mode) throws JniException {
        IhcEncryptionUDF ihcEncUDF = new IhcEncryptionUDF();;
        int keyBits = 64;
        String plainValue = "10000";
        String hexSk = "1F272ECA7DED4105";
        int iterRound = 16;
        String cipherValue = ihcEncUDF.evaluate(mode, hexSk, plainValue);
        System.out.println("cipherValue:" + cipherValue);

        IhcDecryptionUDF ihcDecUDF = new IhcDecryptionUDF();
        String decryptValueStr = ihcDecUDF.evaluate(mode, hexSk, cipherValue);
        System.out.println("decryptValueStr:" + decryptValueStr);
        BigDecimal decryptValue = new BigDecimal(decryptValueStr);
        System.out.println("decryptValue:" + decryptValue);
        byte[] cipherValueBytes = DatatypeConverter.parseHexBinary(cipherValue);
        byte[] cipherValueSum = cipherValueBytes;
        int addRound = 10;
        long startTime = System.currentTimeMillis();
        for (int i = 0; i < addRound - 1; i++) {
            cipherValueSum = NativeFloatingIhc.add(mode, cipherValueSum, cipherValueBytes);
        }
        long endTime = System.currentTimeMillis();
        System.out.println("NativeIhc.add cost time(ms):" + (endTime - startTime));
        String decryptValueSumStr = ihcDecUDF.evaluate(mode, hexSk, DatatypeConverter.printHexBinary(cipherValueSum));
        System.out.println("decryptValueSumStr:" + decryptValueSumStr);
        Assert.assertTrue(new BigDecimal(plainValue).compareTo(decryptValue) == 0);
    }

    @Test
    public void testEncAndDecEvaluate() throws JniException {
        testEncAndDecEvaluateImpl(NativeFloatingIhc.mode.IHC_256.ordinal());
        testEncAndDecEvaluateImpl(NativeFloatingIhc.mode.IHC_128.ordinal());
    }
}
