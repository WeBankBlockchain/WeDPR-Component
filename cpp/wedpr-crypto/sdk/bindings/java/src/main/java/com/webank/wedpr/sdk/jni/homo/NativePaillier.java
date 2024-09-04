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
package com.webank.wedpr.sdk.jni.homo;

import com.webank.wedpr.sdk.jni.common.JniException;
import com.webank.wedpr.sdk.jni.common.JniLibLoader;

public class NativePaillier {
    static {
        JniLibLoader.loadJniLibrary();
    }

    /**
     * get the max length of the public key with given keyBits
     *
     * @param keyBits the keyBits, e.g. 1024/2048
     * @return the max length of the public key
     * @throws JniException
     */
    public static native int maxPublicKeyBytes(int keyBits) throws JniException;

    /**
     * get the max length of the private key with given keyBits
     *
     * @param keyBits the keyBits, e.g. 1024/2048
     * @return the max length of the private key
     * @throws JniException
     */
    public static native int maxPrivateKeyBytes(int keyBits) throws JniException;

    /**
     * get the max length of the cipher with given keyBits
     *
     * @param keyBits the keyBits, e.g. 1024/2048
     * @return the max length of the cipher
     * @throws JniException
     */
    public static native int maxCipherBytes(int keyBits) throws JniException;

    /**
     * get the keyBits according to the given keyPair
     *
     * @param keyPair
     * @return
     * @throws JniException
     */
    public static native int keyBitsFromKeyPair(long keyPair) throws JniException;

    /**
     * get the keyBits according to the given public key
     *
     * @param publicKey
     * @return
     * @throws JniException
     */
    public static native int keyBitsFromPublicKey(long publicKey) throws JniException;

    /**
     * generate the paillier KeyPair
     *
     * @param keyBits the keyLength
     * @return pointer to the generated paillier key-pair
     * @throws JniException
     */
    public static native long generateKeyPair(int keyBits) throws JniException;

    /**
     * load the encoded privateKey and publicKey into key-pair object
     *
     * @param privateKeyBytes the encoded privateKey
     * @param publicKeyBytes the encoded publicKey
     * @return
     * @throws JniException
     */
    public static native long loadKeyPair(byte[] privateKeyBytes, byte[] publicKeyBytes)
            throws JniException;

    /**
     * free the allocated keypair
     *
     * @param keyPairPointer
     * @return
     */
    public static native void freeKeyPair(long keyPairPointer) throws JniException;

    /**
     * load the publicKey object from the encoded publicKey
     *
     * @param publicKeyBytes the encoded publicKey
     * @return the pointer to the publicKey object
     * @throws JniException
     */
    public static native long loadPublicKey(byte[] publicKeyBytes) throws JniException;

    /**
     * free the public key object
     *
     * @param publicKey
     * @throws JniException
     */
    public static native void freePublicKey(long publicKey) throws JniException;

    /**
     * load the private key object from the encoded privateKey
     *
     * @param privateKeyBytes
     * @return
     * @throws JniException
     */
    public static native long loadPrivateKey(byte[] privateKeyBytes) throws JniException;

    /**
     * free the private key object
     *
     * @param privateKey
     * @throws JniException
     */
    public static native void freePrivateKey(long privateKey) throws JniException;

    /**
     * get the public key jni object according to the keypair
     *
     * @param keypair the keypair jni object
     * @return the public key jni object
     * @throws JniException
     */
    public static native long getPublicKeyJniObject(long keypair) throws JniException;

    /**
     * get the encoded publicKey from given keyPair object
     *
     * @param keyPair the pointer to the keyPair object
     * @return
     * @throws JniException
     */
    public static native byte[] getPublicKeyBytesFromKeyPair(long keyPair) throws JniException;

    /**
     * get the encoded privateKey from the given keyPair object
     *
     * @param keyPair the pointer to the keyPair object
     * @return
     * @throws JniException
     */
    public static native byte[] getPrivateKeyBytesFromKeyPair(long keyPair) throws JniException;

    /**
     * encrypt the value into paillier cipher using given keyPair(using CRT optimization)
     *
     * @param value the value to be encrypted
     * @param keyPair the keyPair used to encrypt
     * @return the cipher
     * @throws JniException
     */
    public static native byte[] encryptFast(byte[] value, long keyPair) throws JniException;

    public static native byte[] encryptFastWithoutPrecompute(
            byte[] value, byte[] privateKey, byte[] publicKey) throws JniException;

    /**
     * encrypt the value into paillier cipher using given keyPair(without CRT optimization)
     *
     * @param value the value to be encrypted
     * @param publicKey the publicKey used to encrypt
     * @return the cipher
     * @throws JniException
     */
    public static native byte[] encrypt(byte[] value, long publicKey) throws JniException;

    public static native byte[] encryptWithoutPrecompute(byte[] value, byte[] publicKey)
            throws JniException;

    /**
     * decrypt the cipher using given keyPair
     *
     * @param cipher the cipher to be decrypted
     * @param keyPair the keyPair used to decrypt
     * @return the decrypted result
     * @throws JniException
     */
    public static native byte[] decrypt(byte[] cipher, long keyPair) throws JniException;

    public static native byte[] decryptWithoutPrecompute(byte[] cipher, byte[] sk, byte[] pk)
            throws JniException;

    /**
     * ciphertext space addition, namely: cipher1 + cipher2
     *
     * @param cipher1
     * @param cipher2
     * @param publicKey
     * @return the addition result
     * @throws JniException
     */
    public static native byte[] add(byte[] cipher1, byte[] cipher2, long publicKey)
            throws JniException;

    public static native byte[] addWithoutPrecompute(byte[] cipher1, byte[] cipher2, byte[] pk)
            throws JniException;

    /**
     * ciphertext space subtraction, namely: cipher1 - cipher2
     *
     * @param cipher1
     * @param cipher2
     * @param publicKey
     * @return the subtraction result
     * @throws JniException
     */
    public static native byte[] sub(byte[] cipher1, byte[] cipher2, long publicKey)
            throws JniException;

    public static native byte[] subWithoutPrecompute(byte[] cipher1, byte[] cipher2, byte[] pk)
            throws JniException;

    /**
     * ciphertext space scala-multiply, namely: v * cipher
     *
     * @param v the scala-number
     * @param cipher
     * @param publicKey
     * @return the scala-multiply result
     * @throws JniException
     */
    public static native byte[] scalaMul(byte[] v, byte[] cipher, long publicKey)
            throws JniException;

    public static native byte[] scalaMulWithoutPrecompute(byte[] v, byte[] cipher, byte[] pk)
            throws JniException;
}
