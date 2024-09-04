#include "com_webank_wedpr_sdk_jni_homo_NativePaillier.h"
#include "Common.h"
#include "JNIException.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include <bcos-utilities/Common.h>
/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    maxPublicKeyBytes
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_maxPublicKeyBytes(
    JNIEnv*, jclass, jint keyBits)
{
    return paillier_max_public_key_bytes(keyBits);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    maxPrivateKeyBytes
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_maxPrivateKeyBytes(
    JNIEnv*, jclass, jint keyBits)
{
    return paillier_max_private_key_bytes(keyBits);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    maxCipherBytes
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_maxCipherBytes(
    JNIEnv*, jclass, jint keyBits)
{
    return paillier_max_cipher_bytes(keyBits);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    keyBitsFromKeyPair
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_keyBitsFromKeyPair(
    JNIEnv*, jclass, jlong keypair)
{
    return paillier_key_bits_from_keypair(reinterpret_cast<void*>(keypair));
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    keyBitsFromPublicKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_keyBitsFromPublicKey(
    JNIEnv*, jclass, jlong public_key)
{
    return paillier_key_bits_from_public_key(reinterpret_cast<void*>(public_key));
}
/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    generateKeyPair
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_generateKeyPair(
    JNIEnv* env, jclass, jint keyBits)
{
    auto keyPair = paillier_generate_keypair(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return 0;
    }
    return reinterpret_cast<jlong>(keyPair);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    loadKeyPair
 * Signature: ([B[B)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_loadKeyPair(
    JNIEnv* env, jclass, jbyteArray privateKey, jbyteArray publicKey)
{
    // load key-pair from given private key and public key
    bcos::bytes skBytes;
    auto privateKeyBuffer = convertToInputBuffer(skBytes, env, privateKey);
    bcos::bytes pkBytes;
    auto publicKeyBuffer = convertToInputBuffer(pkBytes, env, publicKey);
    auto keyPair = paillier_load_keypair(&privateKeyBuffer, &publicKeyBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return 0;
    }
    return reinterpret_cast<jlong>(keyPair);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    freeKeyPair
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_freeKeyPair(
    JNIEnv*, jclass, jlong key_pair)
{
    paillier_free_key_pair(reinterpret_cast<void*>(key_pair));
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    loadPublicKey
 * Signature: ([B)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_loadPublicKey(
    JNIEnv* env, jclass, jbyteArray publicKeyBytes)
{
    bcos::bytes pkBytes;
    auto publicKeyBuffer = convertToInputBuffer(pkBytes, env, publicKeyBytes);
    auto publicKeyObj = paillier_load_public_key(&publicKeyBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return 0;
    }
    return reinterpret_cast<jlong>(publicKeyObj);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    freePublicKey
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_freePublicKey(
    JNIEnv*, jclass, jlong public_key)
{
    paillier_free_public_key(reinterpret_cast<void*>(public_key));
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    getPublicKeyJniObject
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_getPublicKeyJniObject(
    JNIEnv*, jclass, jlong jkeyPair)
{
    auto keyPair = reinterpret_cast<void*>(jkeyPair);
    return reinterpret_cast<long>(paillier_get_public_key(keyPair));
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    freePrivateKey
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_freePrivateKey(
    JNIEnv*, jclass, jlong private_key)
{
    paillier_free_public_key(reinterpret_cast<void*>(private_key));
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    getPublicKeyBytesFromKeyPair
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_getPublicKeyBytesFromKeyPair(
    JNIEnv* env, jclass, jlong keypair)
{
    auto publicKeyBuffer =
        paillier_get_public_key_bytes_from_keyPair(reinterpret_cast<void*>(keypair));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(publicKeyBuffer.len);
    env->SetByteArrayRegion(jResult, 0, publicKeyBuffer.len, (jbyte*)publicKeyBuffer.data);
    // release the allocated buffer
    free(publicKeyBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    getPrivateKeyBytesFromKeyPair
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_getPrivateKeyBytesFromKeyPair(
    JNIEnv* env, jclass, jlong keypair)
{
    auto skBuffer = paillier_get_private_key_bytes_from_keypair(reinterpret_cast<void*>(keypair));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(skBuffer.len);
    env->SetByteArrayRegion(jResult, 0, skBuffer.len, (jbyte*)skBuffer.data);
    // release the allocated buffer
    free(skBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    encryptFast
 * Signature: ([BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_encryptFast(
    JNIEnv* env, jclass, jbyteArray value, jlong jkeypair)
{
    auto bigNumValue = JavaBigIntegerToBigNum(env, value);
    auto keypair = reinterpret_cast<void*>(jkeypair);
    // get the keyBits
    auto keyBits = paillier_key_bits_from_keypair(keypair);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    // encrypt
    auto cipherLen = paillier_max_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    paillier_encryt_fast(&cipherBuffer, nullptr, bigNumValue.bn().get(), keypair);
    // copy the cipher into jbyteArray
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    encryptFast
 * Signature: ([B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_encryptFastWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray value, jbyteArray sk, jbyteArray pk)
{
    auto bigNumValue = JavaBigIntegerToBigNum(env, value);
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, pk);
    // Note: pkBuffer.len is not the real keyBytes
    auto cipherLen = paillier_max_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    paillier_encryt_fast_without_precompute(
        &cipherBuffer, nullptr, bigNumValue.bn().get(), &skBuffer, &pkBuffer);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    encrypt
 * Signature: ([BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_encrypt(
    JNIEnv* env, jclass, jbyteArray value, jlong public_key)
{
    auto bigNumValue = JavaBigIntegerToBigNum(env, value);
    auto pk = reinterpret_cast<void*>(public_key);
    // get the keyBits
    auto keyBits = paillier_key_bits_from_public_key(pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // encrypt
    auto cipherLen = paillier_max_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    paillier_encryt(&cipherBuffer, nullptr, bigNumValue.bn().get(), pk);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    encrypt
 * Signature: ([B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_encryptWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray value, jbyteArray pk)
{
    auto bigNumValue = JavaBigIntegerToBigNum(env, value);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, pk);
    // Note: pkBuffer.len is not the real keyBytes
    auto cipherLen = paillier_max_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    paillier_encryt_without_precompute(&cipherBuffer, nullptr, bigNumValue.bn().get(), &pkBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    decrypt
 * Signature: ([BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_decrypt(
    JNIEnv* env, jclass, jbyteArray cipher, jlong keypair)
{
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    auto result = paillier_decrypt(&cipherBuffer, reinterpret_cast<void*>(keypair));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto bigIntegerBytes = BigNumToJavaBigIntegerBytes(env, result);

    if (result)
    {
        BN_clear_free(result);
    }
    return bigIntegerBytes;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    decrypt
 * Signature: ([B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_decryptWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray cipher, jbyteArray sk, jbyteArray pk)
{
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, pk);
    auto result = paillier_decrypt_without_precompute(&cipherBuffer, &skBuffer, &pkBuffer);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto bigIntegerBytes = BigNumToJavaBigIntegerBytes(env, result);
    if (result)
    {
        BN_clear_free(result);
    }
    return bigIntegerBytes;
}


jbyteArray Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_impl(
    JNIEnv* env, jbyteArray cipher1, jbyteArray cipher2, jlong public_key, bool add)
{
    bcos::bytes c1Bytes;
    auto cipher1Buffer = convertToInputBuffer(c1Bytes, env, cipher1);
    bcos::bytes c2Bytes;
    auto cipher2Buffer = convertToInputBuffer(c2Bytes, env, cipher2);
    auto pk = reinterpret_cast<void*>(public_key);

    auto cipherLen = paillier_max_cipher_bytes(paillier_key_bits_from_public_key(pk));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes resultBytes(cipherLen);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    if (add)
    {
        paillier_add(&resultBuffer, &cipher1Buffer, &cipher2Buffer, pk);
    }
    else
    {
        paillier_sub(&resultBuffer, &cipher1Buffer, &cipher2Buffer, pk);
    }

    // call error
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    add
 * Signature: ([B[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_add(
    JNIEnv* env, jclass, jbyteArray cipher1, jbyteArray cipher2, jlong public_key)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_impl(
        env, cipher1, cipher2, public_key, true);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    sub
 * Signature: ([B[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub(
    JNIEnv* env, jclass, jbyteArray cipher1, jbyteArray cipher2, jlong public_key)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_impl(
        env, cipher1, cipher2, public_key, false);
}


jbyteArray Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_without_precompute_impl(
    JNIEnv* env, jbyteArray cipher1, jbyteArray cipher2, jbyteArray public_key, bool add)
{
    bcos::bytes c1Bytes;
    auto cipher1Buffer = convertToInputBuffer(c1Bytes, env, cipher1);
    bcos::bytes c2Bytes;
    auto cipher2Buffer = convertToInputBuffer(c2Bytes, env, cipher2);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, public_key);
    auto cipherLen = paillier_max_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes resultBytes(cipherLen);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    if (add)
    {
        paillier_add_without_precompute(&resultBuffer, &cipher1Buffer, &cipher2Buffer, &pkBuffer);
    }
    else
    {
        paillier_add_without_precompute(&resultBuffer, &cipher1Buffer, &cipher2Buffer, &pkBuffer);
    }
    // call error
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result to jResult
    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    add
 * Signature: ([B[B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_addWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray cipher1, jbyteArray cipher2, jbyteArray pkBytes)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_without_precompute_impl(
        env, cipher1, cipher2, pkBytes, true);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    sub
 * Signature: ([B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_subWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray cipher1, jbyteArray cipher2, jbyteArray pkBytes)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_sub_or_add_without_precompute_impl(
        env, cipher1, cipher2, pkBytes, false);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    scalaMul
 * Signature: ([B[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_scalaMul(
    JNIEnv* env, jclass, jbyteArray value, jbyteArray cipherBytes, jlong public_key)
{
    auto v = JavaBigIntegerToBigNum(env, value);
    bcos::bytes cipherCBytes;
    auto cipherBuffer = convertToInputBuffer(cipherCBytes, env, cipherBytes);
    auto pk = reinterpret_cast<void*>(public_key);

    // allocate the result buffer
    auto cipherLen = paillier_max_cipher_bytes(paillier_key_bits_from_public_key(pk));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes resultBytes(cipherLen);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    // paillier scala_mul
    paillier_scala_mul(&resultBuffer, v.bn().get(), &cipherBuffer, pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    scalMul
 * Signature: ([B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_scalaMulWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray value, jbyteArray cipherBytes, jbyteArray pkBytes)
{
    auto v = JavaBigIntegerToBigNum(env, value);
    bcos::bytes cipherCBytes;
    auto cipherBuffer = convertToInputBuffer(cipherCBytes, env, cipherBytes);
    bcos::bytes pkCBytes;
    auto pkBuffer = convertToInputBuffer(pkCBytes, env, pkBytes);
    auto cipherLen = paillier_max_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes resultBytes(cipherLen);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    // paillier scala_mul
    paillier_scala_mul_without_precompute(&resultBuffer, v.bn().get(), &cipherBuffer, &pkBuffer);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativePaillier
 * Method:    loadPrivateKey
 * Signature: ([B)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativePaillier_loadPrivateKey(
    JNIEnv* env, jclass, jbyteArray privateBytes)
{
    bcos::bytes skBytes;
    auto privateKeyBuffer = convertToInputBuffer(skBytes, env, privateBytes);
    jlong result = reinterpret_cast<long>(paillier_load_private_key(&privateKeyBuffer));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return -1;
    }
    return result;
}