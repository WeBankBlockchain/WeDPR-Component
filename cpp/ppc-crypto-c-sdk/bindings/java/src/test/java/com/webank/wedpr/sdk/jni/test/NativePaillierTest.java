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

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.Utilities;
import com.webank.wedpr.sdk.jni.homo.NativePaillier;
import org.junit.Assert;
import org.junit.Test;

import java.math.BigInteger;
import java.util.Arrays;

public class NativePaillierTest
{
    private void testPaillierImpl(int keyBits, BigInteger m1, BigInteger m2, BigInteger v) throws JniException {
        NativePaillier paillier = new NativePaillier();
        // generate keypair
        long keyPair = paillier.generateKeyPair(keyBits);
        // get the public key from keyPair
        byte[] pk = paillier.getPublicKeyBytesFromKeyPair(keyPair);
        byte[] sk = paillier.getPrivateKeyBytesFromKeyPair(keyPair);
        System.out.println("#### pk: " + Utilities.bytesToHex(pk));
        System.out.println("##### sk: " + Utilities.bytesToHex(sk));

        // load the public key
        long keyPair2 = paillier.loadKeyPair(sk, pk);

        // encrypt
        byte[] m1Bytes = m1.toByteArray();
        System.out.println("###### m1: " + Arrays.toString(m1Bytes));
        byte[] c1 = paillier.encryptFast(m1Bytes, keyPair);
        // encryptFastWithoutPrecompute
        byte[] tmp = paillier.encryptFastWithoutPrecompute(m1Bytes, sk, pk);
        byte[] tmpResult = paillier.decryptWithoutPrecompute(tmp, sk, pk);

        // decrypt
        byte[] resultValue = paillier.decrypt(c1, keyPair);
        BigInteger resultV = new BigInteger(resultValue);
        Assert.assertTrue(Arrays.equals(tmpResult, resultValue));
        tmpResult = paillier.decryptWithoutPrecompute(c1, sk, pk);
        Assert.assertTrue(Arrays.equals(tmpResult, resultValue));

        System.out.println("#### resultValue len: " + resultValue.length + ", resultValue: " + Arrays.toString(resultValue));
        System.out.println("### resultValue: " + resultV);
        System.out.println("#### m1: " + m1);


        byte[] c2 = paillier.encryptFast(m2.toByteArray(), keyPair2);
        System.out.println("#### c1 len: " + c1.length + ", array: " + Arrays.toString(c1));
        System.out.println("#### c2 len: " + c2.length + ", array: " + Arrays.toString(c2));

        /////// c1 + c2
        long pkObj = paillier.loadPublicKey(pk);
        long pkObj2 = paillier.getPublicKeyJniObject(keyPair);

        byte[] result = paillier.add(c1, c2, pkObj);
        tmp = paillier.addWithoutPrecompute(c1, c2, pk);
        System.out.println("#### result len: " + result.length + ", result: " + Arrays.toString(result));
        // decrypt and check
        resultValue = paillier.decrypt(result, keyPair);
        tmpResult = paillier.decryptWithoutPrecompute(tmp, sk, pk);
        Assert.assertTrue(Arrays.equals(resultValue, tmpResult));
        System.out.println("#### resultValue len: " + resultValue.length + ", resultValue: " + Arrays.toString(resultValue));
        BigInteger addResult = new BigInteger(resultValue);
        BigInteger expectedAddResult = m1.add(m2);
        System.out.println("####### m1: " + m1);
        System.out.println("####### m2: " + m2);
        System.out.println("####### expectedAddResult: " + expectedAddResult.toString());
        System.out.println("####### addResult: " + addResult.toString());
        Assert.assertTrue(expectedAddResult.equals(addResult));

        /////// c1 - c2
        result = paillier.sub(c1, c2, pkObj2);
        tmp = paillier.subWithoutPrecompute(c1, c2, pk);
        // decrypt and check
        resultValue = paillier.decrypt(result, keyPair);
        tmpResult = paillier.decryptWithoutPrecompute(result, sk, pk);
        Assert.assertTrue(Arrays.equals(resultValue, tmpResult));
        BigInteger subResult = new BigInteger(resultValue);
        BigInteger expectedSubResult = m1.subtract(m2);
        Assert.assertTrue(expectedSubResult.equals(subResult));

        //// v * c1
        result = paillier.scalaMul(v.toByteArray(), c1, pkObj);
        tmp = paillier.scalaMulWithoutPrecompute(v.toByteArray(), c1, pk);
        // decrypt and check
        resultValue = paillier.decrypt(result, keyPair);
        tmpResult = paillier.decryptWithoutPrecompute(tmp, sk, pk);
        Assert.assertTrue(Arrays.equals(resultValue, tmpResult));
        BigInteger mulResult = new BigInteger(resultValue);
        BigInteger expectedMulResult = v.multiply(m1);
        Assert.assertTrue(mulResult.equals(expectedMulResult));

        /// exception case:
        try {
            c1[1] += 90;
            result = paillier.add(c1, c2, pkObj);
            resultValue = paillier.decrypt(result, keyPair);
            addResult = new BigInteger(resultValue);
            Assert.assertFalse(expectedAddResult.equals(addResult));
        }catch(Exception e)
        {
            System.out.println("#### exception case: " + e.getMessage());
        }

        // call loadPrivateKey
        long skObj = paillier.loadPrivateKey(sk);
        long skObj2 = paillier.loadPrivateKey(sk);

        NativePaillier.freeKeyPair(keyPair);
        NativePaillier.freeKeyPair(keyPair2);

        // Note: pkObj2 is loaded from keypair, it has been released after keypair has been released
        NativePaillier.freePublicKey(pkObj);

        NativePaillier.freePrivateKey(skObj);
        NativePaillier.freePrivateKey(skObj2);
    }

@Test
public void testPaillier() throws JniException {
    // the positive case
    BigInteger m1 =  BigInteger.valueOf(34234234);
    BigInteger m2 =  BigInteger.valueOf(23423423);
    BigInteger v = BigInteger.valueOf(23423);
    testPaillierImpl(2048, m1, m2, v);

    // the negative case
    m1 = BigInteger.valueOf(-34234234);
    testPaillierImpl(2048, m1, m2, v);
}
}