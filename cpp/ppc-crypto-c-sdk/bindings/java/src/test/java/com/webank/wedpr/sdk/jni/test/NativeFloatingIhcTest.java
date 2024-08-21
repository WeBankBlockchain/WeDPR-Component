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

import com.webank.wedpr.sdk.jni.codec.NumberCodec;
import com.webank.wedpr.sdk.jni.codec.NumberCodecException;
import com.webank.wedpr.sdk.jni.codec.NumberCodecImpl;
import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.homo.NativeFloatingIhc;
import org.junit.Assert;
import org.junit.Test;
import com.webank.wedpr.sdk.jni.codec.FloatingPointNumber;


import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Arrays;

public class NativeFloatingIhcTest
{
    private void testFloatingIhcBigDecimalCase(int mode, BigDecimal m1, BigDecimal m2, BigDecimal v) throws JniException, NumberCodecException {
        NativeFloatingIhc ihc = new NativeFloatingIhc();
        // generate key
        byte[] key = ihc.generateKey(mode);
        NumberCodec codec = new NumberCodecImpl();
        FloatingPointNumber fp1 = codec.encode(m1);
        FloatingPointNumber fp2 = codec.encode(m2);
        FloatingPointNumber fv = codec.encode(v);
        // encrypt
        byte[] c1 = ihc.encrypt(mode, key, fp1.getSignificantBytes(),fp1.getExponent());
        // decrypt
        FloatingPointNumber resultValue = ihc.decrypt(mode, key, c1);
        System.out.println("###### resultValue: " + resultValue.getSignificant());
        System.out.println("###### resultValue exp: " + resultValue.getExponent());
        Assert.assertTrue(codec.decodeBigDecimal(resultValue).equals(m1));
        // encrypt c2
        byte[] c2 = ihc.encrypt(mode, key, fp2.getSignificantBytes(),fp2.getExponent());

        /////// c1 + c2
        byte[] addResult = ihc.add(mode, c1, c2);
        // decrypt and check
        resultValue = ihc.decrypt(mode, key, addResult);
        BigDecimal expectedAddResult = m1.add(m2);
        Assert.assertTrue(expectedAddResult.equals(codec.decodeBigDecimal(resultValue)));

        /////// c1 - c2
        byte[] subResult = ihc.sub(mode, c1, c2);
        // decrypt and check
        resultValue = ihc.decrypt(mode, key, subResult);
        BigDecimal expectedSubResult = m1.subtract(m2);
        Assert.assertTrue(expectedSubResult.equals(codec.decodeBigDecimal(resultValue)));

        //// v * c1
        byte[] mulResult = ihc.scalaMul(mode, fv.getSignificantBytes(), fv.getExponent(), c1);
        // decrypt and check
        resultValue = ihc.decrypt(mode, key, mulResult);
        BigDecimal expectedMulResult = v.multiply(m1);
        System.out.println("###### ");
        Assert.assertTrue(expectedMulResult.equals(codec.decodeBigDecimal(resultValue)));

        // invalid case
        String invalidCipher = "invalidCipher";
        Assert.assertThrows(JniException.class, ()->ihc.decrypt(mode, key, invalidCipher.getBytes()));
    }

@Test
public void testFloatingIhc() throws JniException, NumberCodecException {
    System.out.println("##### testFloatingIhc: ");

    BigDecimal m1 = BigDecimal.valueOf(234324.02343243423432434);
    BigDecimal m2 = BigDecimal.valueOf(-324234.0000123423432);
    BigDecimal v =  BigDecimal.valueOf(-1232.000000234);
    testFloatingIhcBigDecimalCase(NativeFloatingIhc.mode.IHC_128.ordinal(), m1, m2, v);
}
}