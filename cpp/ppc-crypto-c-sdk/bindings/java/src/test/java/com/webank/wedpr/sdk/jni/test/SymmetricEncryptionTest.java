/**
 * Copyright 2023 [wedpr]
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.webank.wedpr.sdk.jni.test;
import com.webank.wedpr.sdk.jni.codec.NumberCodecException;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.crypto.FastOre;
import com.webank.wedpr.sdk.jni.crypto.SymmetricEncryption;
import org.junit.Assert;
import org.junit.Test;
import com.webank.wedpr.sdk.jni.common.JniException;

import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class SymmetricEncryptionTest
{
    private void checkEncDec(int algorithm, int mode, byte[] iv, byte[] plain) throws JniException
    {
        byte[] key = SymmetricEncryption.generateKey(algorithm, mode);
        //System.out.println("key: ");
        //String key = "123";
        // encrypt
        byte[] cipher = SymmetricEncryption.encrypt(algorithm, mode, key, iv, plain);
        byte[] decPlain = SymmetricEncryption.decrypt(algorithm, mode, key, iv, cipher);
        // check
        System.out.println("plain: " + new String(plain));
        System.out.println("decPlain: " + new String(decPlain));
        Assert.assertTrue(Arrays.toString(plain).equals(Arrays.toString(decPlain)));
    }
    private void testEncDecCases(int algorithm, int mode, byte[] iv) throws  JniException
    {
        String plain = "231234324234";
        byte[] plainBytes = plain.getBytes();
        System.out.println("#### plainBytes: " + Arrays.toString(plainBytes));
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "@3423sf12!";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "a1b2c3d4e5";

        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "zhongwenzhongw中文";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "中文中文謇謇謇齉齉@#";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "鬱";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "齉";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "躞";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = ".....";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "测试特殊符号*&（……%￥ 123abc 1 12345678 #$%#&$#$*^&(#%$@#()_()";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
        plain = "测试生僻字謇鬱齉躞*&（……%￥ 123abc 12345678 123456782345678 12345678 12345678 12345678 12345678 12345678 12345678 #$%#&$#$*^&(#%$@#()_()";
        checkEncDec(algorithm, mode, iv, plain.getBytes());
    }

    private void testEncDescImpl(byte[] iv) throws JniException
    {
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), iv);

        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_128.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), iv);

        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_192.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), iv);

        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.AES_256.ordinal(), SymmetricEncryption.OperationMode.CTR.ordinal(), iv);

        testEncDecCases(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.TrippleDES.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), iv);

        testEncDecCases(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.ECB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CBC.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.CFB.ordinal(), iv);
        testEncDecCases(SymmetricEncryption.AlgorithmType.SM4.ordinal(), SymmetricEncryption.OperationMode.OFB.ordinal(), iv);
    }
    @Test
    public void testSymmetricEncDec() throws JniException, UnsupportedEncodingException {
        String iv = "123";
        testEncDescImpl(iv.getBytes(StandardCharsets.UTF_8));

        iv = "234324234";
        testEncDescImpl(iv.getBytes(StandardCharsets.UTF_8));
    }

    private void testFastOre4StringImpl(String plain, boolean hex)throws JniException, UnsupportedEncodingException
    {
        byte[] sk = FastOre.generateKey();
        // encrypt
        byte[] cipher = FastOre.encrypt4String(sk, plain.getBytes(), hex);
        // decrypt
        byte[] decryptedValue = FastOre.decrypt4String(sk, cipher, hex);
        Assert.assertEquals(plain, new String(decryptedValue));
    }

    private void testFastOre4NumberImpl(long plain, boolean hex)throws JniException, UnsupportedEncodingException
    {
        byte[] sk = FastOre.generateKey();
        // encrypt
        byte[] cipher = FastOre.encrypt4Integer(sk, plain, hex);
        // decrypt
        long decryptedValue = FastOre.decrypt4Integer(sk, cipher, hex);
        Assert.assertEquals(plain, decryptedValue);
    }

    private void testFastOre4Double(double plain, boolean hex) throws JniException, NumberCodecException {
        NumberCodecImpl numberCodec = new NumberCodecImpl();
        byte[] sk = FastOre.generateKey();
        // encrypt
        byte[] cipher = FastOre.encrypt4Float(sk, numberCodec.toBytes(plain), hex);
        // decrypt
        byte[] decryptedValue = FastOre.decrypt4Float(sk, cipher, hex);

        double EPSILON = 1e-10;
        double decryptedPlain = numberCodec.toDouble(decryptedValue);
        Assert.assertTrue(Math.abs(decryptedPlain - plain) < EPSILON);
    }

    private void testFastOre4Decimal(BigDecimal plain, boolean hex) throws JniException, NumberCodecException {
        NumberCodecImpl numberCodec = new NumberCodecImpl();
        byte[] sk = FastOre.generateKey();
        // encrypt
        byte[] cipher = FastOre.encrypt4Float(sk, numberCodec.toBytes(plain), hex);
        // decrypt
        byte[] decryptedValue = FastOre.decrypt4Float(sk, cipher, hex);
        Assert.assertEquals(0, numberCodec.toBigDecimal(decryptedValue).compareTo(plain));
    }

    @Test
    public void testOre() throws JniException, UnsupportedEncodingException, NumberCodecException {
        String plain = "abcdwer";
        testFastOre4StringImpl(plain, true);
        testFastOre4StringImpl(plain, false);
        plain = "中文中文";
        testFastOre4StringImpl(plain, true);
        testFastOre4StringImpl(plain, false);
        plain = "测试生僻字謇鬱齉躞*&（……%￥";
        testFastOre4StringImpl(plain, true);
        testFastOre4StringImpl(plain, false);

        long number = 0;
        testFastOre4NumberImpl(number, true);
        testFastOre4NumberImpl(number, false);

        for(int i = 0; i < 10000; i++)
        {
            number = -234567;
            testFastOre4NumberImpl(number * i, true);
            testFastOre4NumberImpl(number * i, false);
            number = 876543;
            testFastOre4NumberImpl(number * i, true);
            testFastOre4NumberImpl(number * i, false);
        }

        BigDecimal bigDecimalValue = new BigDecimal("25347862354.00000000000000002345645645000000000000000234342");
        for(int i = 0; i < 10000; i++)
        {
            testFastOre4Double(13425.000001232345435000000012345678 * i, true);
            testFastOre4Double(13425.12345678 * i, false);
            testFastOre4Double(-13425.12345678 * i, true);
            testFastOre4Double(-13425.12345678 * i, false);
            testFastOre4Decimal(bigDecimalValue.multiply(BigDecimal.valueOf(i)), true);
            testFastOre4Decimal(bigDecimalValue.multiply(BigDecimal.valueOf(i)), false);
            testFastOre4Decimal(bigDecimalValue.multiply(BigDecimal.valueOf(-i)), true);
            testFastOre4Decimal(bigDecimalValue.multiply(BigDecimal.valueOf(-i)), false);
        }
    }
}