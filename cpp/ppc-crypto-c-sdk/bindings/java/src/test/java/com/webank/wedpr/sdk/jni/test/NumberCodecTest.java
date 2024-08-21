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
import org.junit.Assert;
import org.junit.Test;

import java.math.BigDecimal;
import java.math.BigInteger;

public class NumberCodecTest
{
    private final NumberCodec codec = new NumberCodecImpl();

    private void checkBigIntegerEncodeDecode(BigInteger value) throws NumberCodecException {
        FloatingPointNumber encodedV = codec.encode(value);
        BigInteger decodedV = codec.decodeBigInteger(encodedV);
        Assert.assertTrue(decodedV.compareTo(value) == 0);
    }

    private void checkLongEncodeDecode(long value) throws NumberCodecException {
        FloatingPointNumber encodedV = codec.encode(value);
        long decodedV = codec.decodeLong(encodedV);
        Assert.assertTrue(decodedV == value);
    }

    private void checkDoubleEncodeDecode(double value) throws NumberCodecException
    {
        FloatingPointNumber encodedV = codec.encode(value);
        double decodedV = codec.decodeDouble(encodedV);
        Assert.assertTrue(Double.compare(value, decodedV) == 0);
    }

    private void checkBigDecimalEncodeDecode(BigDecimal value) throws NumberCodecException
    {
        FloatingPointNumber encodedV = codec.encode(value);
        BigDecimal decodedV = codec.decodeBigDecimal(encodedV);
        Assert.assertTrue(decodedV.compareTo(value) == 0);
    }

    @Test
    public void testNumberCodec() throws NumberCodecException {

        //// check BigInteger encode decode
        BigInteger value = BigInteger.valueOf(123432432);
        checkBigIntegerEncodeDecode(value);
        // negative case
        value = BigInteger.valueOf(-123432432);
        checkBigIntegerEncodeDecode(value);

        /// check long encode/decode
        long longV = 234232454;
        checkLongEncodeDecode(longV);
        /// the negative case
        longV = -234232454;
        checkLongEncodeDecode(longV);

        /// check double
        double doubleV =  11341.234234234;
        checkDoubleEncodeDecode(doubleV);
        // negative case
        doubleV = -1234.234234234;
        checkDoubleEncodeDecode(doubleV);

        /// check BigDecimal
        BigDecimal decimalV = new BigDecimal("2342343243243.2344235235");
        checkBigDecimalEncodeDecode(decimalV);
        decimalV = new BigDecimal("-342343243243.2344235235");
        checkBigDecimalEncodeDecode(decimalV);
    }
}