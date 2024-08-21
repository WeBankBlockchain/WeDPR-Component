package com.webank.wedpr.hive.udf.impl;
import java.nio.charset.StandardCharsets;

import javax.xml.bind.DatatypeConverter;

import com.webank.wedpr.hive.udf.exceptions.DecryptionException;
import com.webank.wedpr.hive.udf.impl.ore.OreDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ore.OreEncryptionUDF;
import com.webank.wedpr.hive.udf.impl.ore.OreNumberDecryptionUDF;
import com.webank.wedpr.hive.udf.impl.ore.OreNumberEncryptionUDF;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import org.apache.commons.codec.Charsets;
import org.apache.commons.codec.binary.Hex;
import org.junit.Assert;
import org.junit.Test;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.crypto.FastOre;

/**
 * @author caryliao
 */

public class OreDecUDFTest {

    @Test
    public void testOREDecUDFEvaluate() throws JniException {
        String plainValue = "19014527865";
        String hexSk = "267C74CB4AFF39E0D59CB177CA5E495A";
        byte[] bytesSk = DatatypeConverter.parseHexBinary(hexSk);
        byte[] bytesCipher = FastOre.encrypt4String(bytesSk, plainValue.getBytes(StandardCharsets.UTF_8), true);
        System.out.println("hexCipher: " + new String(bytesCipher));

        OreDecryptionUDF oreDecUDF = new OreDecryptionUDF();
        long startTime = System.nanoTime();
        String decryptValueStr = oreDecUDF.evaluate(new String(bytesCipher), hexSk);
        long endTime = System.nanoTime();
        System.out.println("OREDecUDF cost time(ns):" + (endTime - startTime));
        System.out.println("decryptValueStr:" + decryptValueStr);
        Assert.assertEquals(plainValue,decryptValueStr);
    }

    public void testOREDecUDFEvaluateImpl2(String plainValue, String hexSk, byte[] cipher){
        OreDecryptionUDF oreDecUDF = new OreDecryptionUDF();
        String decryptValueStr = oreDecUDF.evaluate(new String(cipher), hexSk);
        System.out.println("decryptValueStr:" + decryptValueStr);
        System.out.println("##### decryptValueStr: " + decryptValueStr);
        System.out.println("##### plainValue:" + plainValue);
        Assert.assertEquals(plainValue, decryptValueStr);
    }

    public void checkInvalidCase( String hexSk, byte[] cipher)
    {
        OreDecryptionUDF oreDecUDF = new OreDecryptionUDF();
        Assert.assertThrows(DecryptionException.class, ()-> oreDecUDF.evaluate(new String(cipher), hexSk));
    }
    
    @Test
    public void testOREDecUDFEvaluate3() throws JniException{
        String plainValue = "456_world";
        String hexSk = "267C74CB4AFF39E0D59CB177CA5E495A";
        testOreEncDecImpl(plainValue, hexSk);

        plainValue = "202.103.123.99";
        String cipher = "af067bca0937b7420a60e45de9831bd5b70ae0f950b6d748c470da10";
        testOREDecUDFEvaluateImpl2(plainValue, hexSk, cipher.getBytes(Charsets.UTF_8));
        plainValue = "192.168.19.12";
        cipher = "af05856a300f0bee890821b3a560af45b537a7e561b606252eed";
        testOREDecUDFEvaluateImpl2(plainValue, hexSk, cipher.getBytes(Charsets.UTF_8));

        checkInvalidCase("abc", cipher.getBytes(Charsets.UTF_8));
        cipher = "abcwsd";
        checkInvalidCase(hexSk, cipher.getBytes(Charsets.UTF_8));
    }

    public void testOreEncDecImpl(String plainData, String hexSk) {
        OreEncryptionUDF oreEncUDF = new OreEncryptionUDF();
        OreDecryptionUDF oreDecUDF = new OreDecryptionUDF();

        String tmpCipher = "";
        for (int i = 0; i < 10; i++)
        {
            String cipher = oreEncUDF.evaluate(plainData, hexSk);
            if(i >=1)
            {
                Assert.assertEquals(cipher, tmpCipher);
            }
            tmpCipher = cipher;
            String plain = oreDecUDF.evaluate(cipher, hexSk);
            Assert.assertEquals(plain, plainData);
        }
    }

    @Test
    public void testOreEncDecEvaluate() throws JniException
    {
        byte[] sk = FastOre.generateKey();
        String hexSk = Hex.encodeHexString(sk);
        testOreEncDecImpl("abc", hexSk);
        testOreEncDecImpl("中国werw34#$%546!!@3中国中文", hexSk);
        testOreEncDecImpl("192.168.19.12", hexSk);
        testOreEncDecImpl("202.103.123.99", hexSk);
    }


    public void testOreNumberEncDecImpl(String plainData) {
        OreNumberEncryptionUDF oreEncUDF = new OreNumberEncryptionUDF();
        OreNumberDecryptionUDF oreDecUDF = new OreNumberDecryptionUDF();

        String tmpCipher = "";
        for (int i = 0; i < 10; i++)
        {
            String cipher = oreEncUDF.evaluate(plainData);
            if(i >=1)
            {
                Assert.assertEquals(cipher, tmpCipher);
            }
            tmpCipher = cipher;
            String plain = oreDecUDF.evaluate(cipher);
            System.out.println("#### plain: " + plain);
            System.out.println("#### plainData: " + plainData);
            Assert.assertEquals(plain, plainData);
        }
    }

    @Test
    public void testOreNumberEncDecEvaluate() throws JniException
    {
        testOreNumberEncDecImpl("12332432423.23432444444444");
        testOreNumberEncDecImpl("-12332432423.23432444444444");
        testOreNumberEncDecImpl("12332432423");
        testOreNumberEncDecImpl("-12332432423");
        testOreNumberEncDecImpl("0");
        testOreNumberEncDecImpl("0.000000000000000000000000000000001");
        testOreNumberEncDecImpl("-0.000000000000000000000000000000001");
    }
}
