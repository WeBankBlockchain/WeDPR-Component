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
package com.webank.wedpr.sdk.jni.codec;

import java.math.BigDecimal;
import java.math.BigInteger;

public class NumberCodecImpl implements NumberCodec {
    private static final int BASE = 10;
    /**
     * encode the given BigInteger into EncodedValue
     *
     * @param value
     * @return
     */
    public FloatingPointNumber encode(BigInteger value) throws NumberCodecException {
        if (value == null) {
            throw new NumberCodecException("Not support encode null BigInteger!");
        }
        return new FloatingPointNumber(value, 0);
    }

    /**
     * encode the given long value into EncodedValue
     *
     * @param value
     * @return
     * @throws NumberCodecException
     */
    public FloatingPointNumber encode(long value) throws NumberCodecException {
        return encode(BigInteger.valueOf(value));
    }

    /**
     * encode the given big-decimal into EncodedValue
     *
     * @param value
     * @return
     * @throws NumberCodecException
     */
    public FloatingPointNumber encode(BigDecimal value) throws NumberCodecException {
        if (value == null) {
            throw new NumberCodecException("Not support encode null BigDecimal!");
        }
        BigInteger significant;
        int exp = -value.scale();
        if (value.scale() > 0) {
            significant = value.scaleByPowerOfTen(value.scale()).toBigInteger();
        } else {
            significant = value.unscaledValue();
        }
        return new FloatingPointNumber(significant, exp);
    }

    public FloatingPointNumber encode(double value) throws NumberCodecException {
        if (Double.isInfinite(value) || Double.isNaN(value)) {
            throw new NumberCodecException("Not support encode null/infinite double!");
        }
        // convert double to BigDecimal
        BigDecimal decimalValue = BigDecimal.valueOf(value);
        return encode(decimalValue);
    }

    /**
     * decode the encodedValue into BigInteger
     *
     * @param encodedValue
     * @return
     * @throws NumberCodecException
     */
    public BigInteger decodeBigInteger(FloatingPointNumber encodedValue)
            throws NumberCodecException {
        return encodedValue.getSignificant();
    }

    /**
     * decode the encodedValue into long
     *
     * @param encodedValue
     * @return
     * @throws NumberCodecException
     */
    public long decodeLong(FloatingPointNumber encodedValue) throws NumberCodecException {
        BigInteger value = decodeBigInteger(encodedValue);
        // check overflow or not
        if (value.compareTo(BigInteger.valueOf(Long.MIN_VALUE)) < 0
                || value.compareTo(BigInteger.valueOf(Long.MAX_VALUE)) > 0) {
            throw new NumberCodecException(
                    "the Decoded value can't be represented as long, decoded value: "
                            + value.toString());
        }
        return value.longValue();
    }

    /**
     * decode the given encodedValue into BigDecimal
     *
     * @param encodedValue
     * @return
     * @throws NumberCodecException
     */
    public BigDecimal decodeBigDecimal(FloatingPointNumber encodedValue)
            throws NumberCodecException {
        BigInteger significant = encodedValue.getSignificant();
        return new BigDecimal(significant, -encodedValue.getExponent());
    }

    /**
     * decode the encodedValue into double
     *
     * @param encodedValue
     * @return
     * @throws NumberCodecException
     */
    public double decodeDouble(FloatingPointNumber encodedValue) throws NumberCodecException {
        return decodeBigDecimal(encodedValue).doubleValue();
    }

    @Override
    public byte[] toBytes(BigDecimal value) {
        return value.toPlainString().getBytes();
    }

    @Override
    public byte[] toBytes(Double value) {
        return String.valueOf(value).getBytes();
    }

    @Override
    public double toDouble(byte[] value) {
        return Double.parseDouble(new String(value));
    }

    @Override
    public BigDecimal toBigDecimal(byte[] value) {
        return new BigDecimal(new String(value));
    }
}
