package com.webank.wedpr.hive.udf.impl;

import java.nio.charset.StandardCharsets;

import javax.xml.bind.DatatypeConverter;

import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.hive.udf.impl.symmetric.SymDecryptionUDF;
import org.junit.Assert;
import org.junit.Test;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;

/**
 * @author caryliao
 * @ 2023/11/1
 */

public class SymDecUDFTest {
    @Test
    public void testSymDecUDFEvaluate() throws JniException {
        doTest(SymmetricEncryption.AlgorithmType.AES_128, SymmetricEncryption.OperationMode.ECB);
        long startTime = System.nanoTime();
        doTest(SymmetricEncryption.AlgorithmType.AES_128, SymmetricEncryption.OperationMode.CTR);
        long endTime = System.nanoTime();
        System.out.println("AES_128 CTR cost time(ns):" + (endTime - startTime));
        doTest(SymmetricEncryption.AlgorithmType.AES_192, SymmetricEncryption.OperationMode.CBC);
        doTest(SymmetricEncryption.AlgorithmType.AES_256, SymmetricEncryption.OperationMode.OFB);
        doTest(SymmetricEncryption.AlgorithmType.AES_256, SymmetricEncryption.OperationMode.CFB);
        doTest(SymmetricEncryption.AlgorithmType.AES_256, SymmetricEncryption.OperationMode.CTR);

        doTest(SymmetricEncryption.AlgorithmType.TrippleDES, SymmetricEncryption.OperationMode.ECB);
        doTest(SymmetricEncryption.AlgorithmType.TrippleDES, SymmetricEncryption.OperationMode.CBC);
        doTest(SymmetricEncryption.AlgorithmType.TrippleDES, SymmetricEncryption.OperationMode.OFB);
        doTest(SymmetricEncryption.AlgorithmType.TrippleDES, SymmetricEncryption.OperationMode.CFB);

        doTest(SymmetricEncryption.AlgorithmType.SM4, SymmetricEncryption.OperationMode.ECB);
        doTest(SymmetricEncryption.AlgorithmType.SM4, SymmetricEncryption.OperationMode.CBC);
        doTest(SymmetricEncryption.AlgorithmType.SM4, SymmetricEncryption.OperationMode.OFB);
        doTest(SymmetricEncryption.AlgorithmType.SM4, SymmetricEncryption.OperationMode.CFB);
        doTest(SymmetricEncryption.AlgorithmType.SM4, SymmetricEncryption.OperationMode.CTR);
    }

    private void doTest(SymmetricEncryption.AlgorithmType algorithmType,
            SymmetricEncryption.OperationMode operationMode) throws JniException {
        String hexIv = "30313233343536373839616263646566";
        String hexSk = "8ff6bbbd8b5f79091cbf8e4676b7ff81";
        byte[] bytesIv = DatatypeConverter.parseHexBinary(hexIv);
        byte[] bytesSk = DatatypeConverter.parseHexBinary(hexSk);
        String plainValue = "caryliao";
        byte[] bytesCipher = SymmetricEncryption.encrypt(algorithmType.ordinal(), operationMode.ordinal(), bytesSk,
                bytesIv, plainValue.getBytes(StandardCharsets.UTF_8));
        String hexCipher = DatatypeConverter.printHexBinary(bytesCipher);
        System.out.println("hexCipher:" + hexCipher);

        SymDecryptionUDF symDecUDF = new SymDecryptionUDF();
        String decryptValueStr =
                symDecUDF.evaluate(hexCipher, algorithmType.ordinal(), operationMode.ordinal(), hexIv, hexSk);
        System.out.println("decryptValueStr:" + decryptValueStr);
        Assert.assertTrue(plainValue.equals(decryptValueStr));

        // evaluate with invalid cipher
        Assert.assertThrows(DecryptionException.class,
                 () -> symDecUDF.evaluate("abc1wer", algorithmType.ordinal(), operationMode.ordinal(), hexIv, hexSk));
    }

}
