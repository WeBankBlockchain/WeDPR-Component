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

import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;
import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecException;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingPointPaillier;
import com.webank.wedpr.sdk.jni.homo.NativePaillier;
import org.junit.Assert;
import org.junit.Test;

import java.math.BigDecimal;
import java.math.BigInteger;

public class NativeFloatingPointPaillierTest
{
    private NumberCodec codec = new NumberCodecImpl();
    private void intCase(BigInteger m1, BigInteger m2, BigInteger v, long keypair, long publicKey) throws NumberCodecException, JniException {
        // encrypt
        FloatingPointNumber m1Fp = codec.encode(m1);
        byte[] c1 = NativeFloatingPointPaillier.encrypt(m1Fp.getSignificantBytes(), m1Fp.getExponent(), publicKey);
        byte[] pkBytes = NativePaillier.getPublicKeyBytesFromKeyPair(keypair);
        byte[] skBytes = NativePaillier.getPrivateKeyBytesFromKeyPair(keypair);
        byte[] tmp = NativeFloatingPointPaillier.encryptWithoutPrecompute(m1Fp.getSignificantBytes(), m1Fp.getExponent(), pkBytes);

        FloatingPointNumber m2Fp = codec.encode(m2);
        byte[] c2 = NativeFloatingPointPaillier.encryptFast(m2Fp.getSignificantBytes(), m2Fp.getExponent(), keypair);
        // decrypt and check
        FloatingPointNumber decryptedC1 = NativeFloatingPointPaillier.decrypt(c1, keypair);
        FloatingPointNumber tmpResult = NativeFloatingPointPaillier.decryptWithoutPrecompute(tmp, skBytes, pkBytes);
        Assert.assertTrue(codec.decodeBigInteger(decryptedC1).compareTo(m1) == 0);
        Assert.assertTrue(codec.decodeBigInteger(tmpResult).compareTo(m1) == 0);

        FloatingPointNumber decryptedC2 = NativeFloatingPointPaillier.decrypt(c2, keypair);
        Assert.assertTrue(codec.decodeBigInteger(decryptedC2).compareTo(m2) == 0);

        // add
        byte[] addCipher = NativeFloatingPointPaillier.add(c1, c2, publicKey);
        tmp = NativeFloatingPointPaillier.addWithoutPrecompute(c1, c2, pkBytes);
        FloatingPointNumber addResult = NativeFloatingPointPaillier.decrypt(addCipher, keypair);
        tmpResult = NativeFloatingPointPaillier.decryptWithoutPrecompute(tmp, skBytes, pkBytes);

        BigInteger expectedAddResult = m1.add(m2);
        Assert.assertTrue(codec.decodeBigInteger(addResult).compareTo(expectedAddResult) == 0);
        Assert.assertTrue(codec.decodeBigInteger(tmpResult).compareTo(expectedAddResult) == 0);

        // sub
        byte[] subCipher = NativeFloatingPointPaillier.sub(c1, c2, publicKey);
        tmp = NativeFloatingPointPaillier.subWithoutPrecompute(c1, c2, pkBytes);
        FloatingPointNumber subResult = NativeFloatingPointPaillier.decrypt(subCipher, keypair);
        tmpResult = NativeFloatingPointPaillier.decryptWithoutPrecompute(subCipher, skBytes, pkBytes);
        BigInteger expectedSubResult = m1.subtract(m2);
        Assert.assertTrue(codec.decodeBigInteger(subResult).compareTo(expectedSubResult) == 0);
        Assert.assertTrue(codec.decodeBigInteger(tmpResult).compareTo(expectedSubResult) == 0);

        // scalaMul
        FloatingPointNumber vFp = codec.encode(v);
        byte[] mulCipher = NativeFloatingPointPaillier.scalaMul(vFp.getSignificantBytes(), vFp.getExponent(), c1, publicKey);
        tmp = NativeFloatingPointPaillier.scalaMulWithoutPrecompute(vFp.getSignificantBytes(), vFp.getExponent(), c1, pkBytes);
        FloatingPointNumber mulResult = NativeFloatingPointPaillier.decrypt(mulCipher, keypair);
        tmpResult = NativeFloatingPointPaillier.decryptWithoutPrecompute(mulCipher, skBytes, pkBytes);
        BigInteger expectedMulResult = v.multiply(m1);
        Assert.assertTrue(codec.decodeBigInteger(mulResult).compareTo(expectedMulResult) == 0);
        Assert.assertTrue(codec.decodeBigInteger(tmpResult).compareTo(expectedMulResult) == 0);
    }

    private void doubleCase(double m1, double m2, double v, long keypair, long publicKey) throws NumberCodecException, JniException {
        // encrypt
        FloatingPointNumber m1Fp = codec.encode(m1);
        System.out.println("#### m1Fp: " + m1Fp.toString());
        byte[] c1 = NativeFloatingPointPaillier.encrypt(m1Fp.getSignificantBytes(), m1Fp.getExponent(), publicKey);

        FloatingPointNumber m2Fp = codec.encode(m2);
        System.out.println("#### m2Fp: " + m2Fp.toString());
        byte[] c2 = NativeFloatingPointPaillier.encryptFast(m2Fp.getSignificantBytes(), m2Fp.getExponent(), keypair);
        // decrypt and check
        FloatingPointNumber decryptedC1 = NativeFloatingPointPaillier.decrypt(c1, keypair);
        Assert.assertTrue(Double.compare(codec.decodeDouble(decryptedC1), m1) == 0);

        FloatingPointNumber decryptedC2 = NativeFloatingPointPaillier.decrypt(c2, keypair);
        System.out.println("#### m2: " + m2);
        System.out.println("#### decryptedC2: " + codec.decodeDouble(decryptedC2));
        Assert.assertTrue(Double.compare(codec.decodeDouble(decryptedC2), m2) == 0);

        // add
        byte[] addCipher = NativeFloatingPointPaillier.add(c1, c2, publicKey);
        FloatingPointNumber addResult = NativeFloatingPointPaillier.decrypt(addCipher, keypair);
        BigDecimal expectedAddResult = BigDecimal.valueOf(m1).add(BigDecimal.valueOf(m2));
        System.out.println("#### addResult: " + addResult.toString());
        System.out.println("#### expectedAddResult: " + expectedAddResult);
        System.out.println("m1: " + m1);
        System.out.println("m2: " + m2);
        Assert.assertTrue(expectedAddResult.compareTo(BigDecimal.valueOf(codec.decodeDouble(addResult))) == 0);

        // sub
        byte[] subCipher = NativeFloatingPointPaillier.sub(c1, c2, publicKey);
        FloatingPointNumber subResult = NativeFloatingPointPaillier.decrypt(subCipher, keypair);
        BigDecimal expectedSubResult = BigDecimal.valueOf(m1).subtract(BigDecimal.valueOf(m2));
        Assert.assertTrue(expectedSubResult.compareTo(BigDecimal.valueOf(codec.decodeDouble(subResult))) == 0);

        // scalaMul
        FloatingPointNumber vFp = codec.encode(v);
        byte[] mulCipher = NativeFloatingPointPaillier.scalaMul(vFp.getSignificantBytes(), vFp.getExponent(), c1, publicKey);
        FloatingPointNumber mulResult = NativeFloatingPointPaillier.decrypt(mulCipher, keypair);
        BigDecimal expectedMulResult = BigDecimal.valueOf(v).multiply(BigDecimal.valueOf(m1));
        // Note: BigDecimal not loss the precision
        Assert.assertTrue(expectedMulResult.compareTo(codec.decodeBigDecimal(mulResult)) == 0);
    }

    private void decimalCase(BigDecimal m1, BigDecimal m2, BigDecimal v, long keypair, long publicKey) throws NumberCodecException, JniException {
        // encrypt
        FloatingPointNumber m1Fp = codec.encode(m1);
        byte[] c1 = NativeFloatingPointPaillier.encrypt(m1Fp.getSignificantBytes(), m1Fp.getExponent(), publicKey);

        FloatingPointNumber m2Fp = codec.encode(m2);
        byte[] c2 = NativeFloatingPointPaillier.encryptFast(m2Fp.getSignificantBytes(), m2Fp.getExponent(), keypair);
        // decrypt and check
        FloatingPointNumber decryptedC1 = NativeFloatingPointPaillier.decrypt(c1, keypair);
        Assert.assertTrue(codec.decodeBigDecimal(decryptedC1).compareTo(m1) == 0);

        FloatingPointNumber decryptedC2 = NativeFloatingPointPaillier.decrypt(c2, keypair);
        Assert.assertTrue(codec.decodeBigDecimal(decryptedC2).compareTo(m2) == 0);

        // add
        byte[] addCipher = NativeFloatingPointPaillier.add(c1, c2, publicKey);
        FloatingPointNumber addResult = NativeFloatingPointPaillier.decrypt(addCipher, keypair);
        BigDecimal expectedAddResult = m1.add(m2);
        Assert.assertTrue(codec.decodeBigDecimal(addResult).compareTo(expectedAddResult) == 0);

        // sub
        byte[] subCipher = NativeFloatingPointPaillier.sub(c1, c2, publicKey);
        FloatingPointNumber subResult = NativeFloatingPointPaillier.decrypt(subCipher, keypair);
        BigDecimal expectedSubResult = m1.subtract(m2);
        Assert.assertTrue(codec.decodeBigDecimal(subResult).compareTo(expectedSubResult) == 0);

        // scalaMul
        FloatingPointNumber vFp = codec.encode(v);
        byte[] mulCipher = NativeFloatingPointPaillier.scalaMul(vFp.getSignificantBytes(), vFp.getExponent(), c1, publicKey);
        FloatingPointNumber mulResult = NativeFloatingPointPaillier.decrypt(mulCipher, keypair);
        BigDecimal expectedMulResult = v.multiply(m1);
        Assert.assertTrue(codec.decodeBigDecimal(mulResult).compareTo(expectedMulResult) == 0);
    }

    @Test
    public void testIntCase() throws JniException, NumberCodecException {
        /// the int case
        BigInteger m1 = BigInteger.valueOf(234234234);
        BigInteger m2 = BigInteger.valueOf(456566423);
        BigInteger v = BigInteger.valueOf(123);
        int keyBits = 2048;
        long keypair = NativePaillier.generateKeyPair(keyBits);
        long publicKey = NativePaillier.getPublicKeyJniObject(keypair);

        // the positive case
        intCase(m1, m2, v, keypair, publicKey);
        // the negative case
        m2 = BigInteger.valueOf(-456566423);
        intCase(m1, m2, v, keypair, publicKey);
        m1 = BigInteger.valueOf(-234234234);
        intCase(m1, m2, v, keypair,publicKey);

        // zero caase
        m1 = BigInteger.valueOf(0);
        m2 = BigInteger.valueOf(0);
        intCase(m1, m2, v, keypair,publicKey);
    }

    @Test
    public void testDoubleCase() throws JniException, NumberCodecException {
        int keyBits = 2048;
        long keypair = NativePaillier.generateKeyPair(keyBits);
        long publicKey = NativePaillier.getPublicKeyJniObject(keypair);

        /// positive case
        double m1 = 45646.00;
        double m2 = 0.454354;
        double v = 2.00;
        doubleCase(m1, m2, v, keypair, publicKey);

        // negative case
        m1 = -234324.0234324;
        m2 = -0.34543;
        v = -23432.001;
        doubleCase(m1, m2, v, keypair, publicKey);

        // zero case
        //m1 = 0.000;
        m2 = 0.00;
        doubleCase(m1, m2, v, keypair, publicKey);
    }

    @Test
    public void testBigDecimalCase() throws JniException, NumberCodecException {
        int keyBits = 2048;
        long keypair = NativePaillier.generateKeyPair(keyBits);
        long publicKey = NativePaillier.getPublicKeyJniObject(keypair);

        BigDecimal m1 = BigDecimal.valueOf(12321231.2342343);
        BigDecimal m2 = BigDecimal.valueOf(0.0023434);
        BigDecimal v = BigDecimal.valueOf(123.3);
        decimalCase(m1, m2, v, keypair, publicKey);
        m1 = new BigDecimal("2343243223423423423423.798034320080");
        m2 = new BigDecimal("956456547424236462412342345.0023423");
        decimalCase(m1, m2, v, keypair, publicKey);

        // the negative case
        m1 = BigDecimal.valueOf(234324.023432434);
        m2 = BigDecimal.valueOf(-324234.00001);
        v =  BigDecimal.valueOf(-1232.00);
        decimalCase(m1, m2, v, keypair, publicKey);
        v = BigDecimal.valueOf(0.23434);
        m1 = BigDecimal.valueOf(-43543.000);
        decimalCase(m1, m2, v, keypair, publicKey);
        m2 = BigDecimal.valueOf(-2343.00);
        decimalCase(m1, m2, v, keypair, publicKey);

        // zero case
        m1 = BigDecimal.valueOf(0);
        m2 = BigDecimal.valueOf(0);
        decimalCase(m1, m2, v, keypair, publicKey);
    }

}