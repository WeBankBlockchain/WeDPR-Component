package com.webank.wedpr.hive.udf.impl;

import java.math.BigDecimal;

import com.webank.wedpr.hive.udf.impl.paillier.PaillierDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.paillier.PaillierEncryptionUDF;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingPointPaillier;
import org.apache.hadoop.io.Text;
import org.junit.Assert;
import org.junit.Test;

/**
 * @author caryliao
 */

public class PaillierDecUDFTest {
    @Test
    public void testPaillierDecryptionUDFEvaluate() {
        String plain = "1600.7568000000001120497472584247589111328125";
        PaillierEncryptionUDF paillierEncryptionUDF = new PaillierEncryptionUDF();
        Text hexCipher = paillierEncryptionUDF.evaluate(new Text(plain));

        PaillierDecryptionUDF paillierDecUDF = new PaillierDecryptionUDF();
        long startTime = System.nanoTime();
        String decryptValueStr =
                paillierDecUDF.evaluate(hexCipher);
        long endTime = System.nanoTime();
        System.out.println("paillierDecUDF cost time(ns):" + (endTime - startTime));
        String decryptValueStr2 =
                paillierDecUDF.evaluate(hexCipher);
        System.out.println("decryptValueStr:" + decryptValueStr);
        System.out.println("decryptValueStr2:" + decryptValueStr2);
        BigDecimal decryptValue = new BigDecimal(decryptValueStr);
        BigDecimal decryptValue2 = new BigDecimal(decryptValueStr2);
        System.out.println("##### decryptValue: " + decryptValueStr);
        System.out.println("##### decryptValue2: " + decryptValueStr2);
        Assert.assertTrue(new BigDecimal(plain).compareTo(decryptValue) == 0);
        Assert.assertTrue(new BigDecimal(plain).compareTo(decryptValue2) == 0);
    }

}
