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

import java.math.BigInteger;
import java.util.Objects;

// the floating-point-value
public class FloatingPointNumber {
    private final BigInteger significant;
    private final int exponent;

    FloatingPointNumber(byte[] significantBytes, int exponent) {
        this.significant = new BigInteger(significantBytes);
        this.exponent = exponent;
    }

    public FloatingPointNumber(BigInteger significant, int exponent) {
        this.significant = significant;
        this.exponent = exponent;
    }

    public BigInteger getSignificant() {
        return significant;
    }

    public byte[] getSignificantBytes() {
        return significant.toByteArray();
    }

    public int getExponent() {
        return exponent;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        FloatingPointNumber that = (FloatingPointNumber) o;
        return exponent == that.exponent && Objects.equals(significant, that.significant);
    }

    //    @Override
    public FloatingPointNumber add(FloatingPointNumber other) {
        if (this.exponent == other.exponent) {
            return new FloatingPointNumber(
                    this.getSignificant().add(other.getSignificant()), this.exponent);
        }
        if (this.exponent < other.exponent) {
            int diffExp = other.exponent - this.exponent;
            BigInteger baseC = BigInteger.valueOf(10).pow(diffExp);
            FloatingPointNumber otherNew =
                    new FloatingPointNumber(other.getSignificant().multiply(baseC), this.exponent);
            return new FloatingPointNumber(
                    otherNew.getSignificant().add(this.getSignificant()), otherNew.exponent);
        }
        int diffExp = this.exponent - other.exponent;
        BigInteger baseC = BigInteger.valueOf(10).pow(diffExp);
        FloatingPointNumber thisNew =
                new FloatingPointNumber(this.getSignificant().multiply(baseC), other.exponent);
        return new FloatingPointNumber(
                thisNew.getSignificant().add(other.getSignificant()), thisNew.exponent);
    }

    public FloatingPointNumber sub(FloatingPointNumber other) {
        if (this.exponent == other.exponent) {
            return new FloatingPointNumber(
                    this.getSignificant().add(other.getSignificant()), this.exponent);
        }
        if (this.exponent < other.exponent) {
            int diffExp = other.exponent - this.exponent;
            BigInteger baseC = BigInteger.valueOf(10).pow(diffExp);
            FloatingPointNumber otherNew =
                    new FloatingPointNumber(other.getSignificant().multiply(baseC), this.exponent);
            return new FloatingPointNumber(
                    otherNew.getSignificant().subtract(this.getSignificant()), otherNew.exponent);
        }
        int diffExp = this.exponent - other.exponent;
        BigInteger baseC = BigInteger.valueOf(10).pow(diffExp);
        FloatingPointNumber thisNew =
                new FloatingPointNumber(this.getSignificant().multiply(baseC), other.exponent);
        return new FloatingPointNumber(
                thisNew.getSignificant().subtract(other.getSignificant()), thisNew.exponent);
    }

    @Override
    public int hashCode() {
        return Objects.hash(significant, exponent);
    }

    @Override
    public String toString() {
        return "FloatingPointNumber{"
                + "significant="
                + significant
                + ", exponent="
                + exponent
                + '}';
    }
}
