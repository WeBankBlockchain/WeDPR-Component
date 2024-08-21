#include "com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier.h"
#include "Common.h"
#include "JNIException.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    maxCipherBytes
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_maxCipherBytes(
    JNIEnv*, jclass, jint keyBits)
{
    return floating_point_paillier_cipher_bytes(keyBits);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    encryptFast
 * Signature: ([BIJ)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_encryptFast(
    JNIEnv* env, jclass, jbyteArray significantBytes, jint exponent, jlong jkeypair)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);

    auto keypair = reinterpret_cast<void*>(jkeypair);
    auto keyBits = paillier_key_bits_from_keypair(keypair);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto cipherLen = floating_point_paillier_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};

    floating_point_paillier_encrypt_fast(&cipherBuffer, significant.bn().get(), exponent, keypair);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    encryptFastWithoutPrecompute
 * Signature: ([BI[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_encryptFastWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray significantBytes, jint exponent, jbyteArray sk, jbyteArray pk)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);

    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, pk);
    auto cipherLen = floating_point_paillier_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};

    floating_point_paillier_encrypt_fast_without_precompute(
        &cipherBuffer, significant.bn().get(), exponent, &skBuffer, &pkBuffer);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    encrypt
 * Signature: ([BIJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_encrypt(
    JNIEnv* env, jclass, jbyteArray significantBytes, jint exponent, jlong jpublicKey)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);

    auto pk = reinterpret_cast<void*>(jpublicKey);
    auto keyBits = paillier_key_bits_from_public_key(pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto cipherLen = floating_point_paillier_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    floating_point_paillier_encrypt(&cipherBuffer, significant.bn().get(), exponent, pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    encryptWithoutPrecompute
 * Signature: ([BI[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_encryptWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray significantBytes, jint exponent, jbyteArray pkBytes)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);

    bcos::bytes pkCBytes;
    auto pkBuffer = convertToInputBuffer(pkCBytes, env, pkBytes);
    auto cipherLen = floating_point_paillier_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    floating_point_paillier_encrypt_without_precompute(
        &cipherBuffer, significant.bn().get(), exponent, &pkBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // copy the result
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    decrypt
 * Signature: ([BJ)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_decrypt(
    JNIEnv* env, jclass, jbyteArray cipher, jlong jkeypair)
{
    ppc::crypto::BigNum decrypted_significant;
    int16_t exponent;
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    floating_point_paillier_decrypt(decrypted_significant.bn().get(), &exponent, &cipherBuffer,
        reinterpret_cast<void*>(jkeypair));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // convert to FloatingPointNumber
    jclass fpNumberClass = env->FindClass("com/webank/wedpr/sdk/jni/codec/FloatingPointNumber");
    // FloatingPointNumber(byte[] significantBytes, int exponent)
    jmethodID initFunID = env->GetMethodID(fpNumberClass, "<init>", "([BI)V");
    jbyteArray plainBytes = BigNumToJavaBigIntegerBytes(env, decrypted_significant.bn().get());
    jobject result = env->NewObject(fpNumberClass, initFunID, plainBytes, exponent);
    env->DeleteLocalRef(plainBytes);
    return result;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    decryptWithoutPrecompute
 * Signature: ([B[B[B)Lcom/webank/wedpr/sdk/jni/codec/FloatingPointNumber;
 */
JNIEXPORT jobject JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_decryptWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray cipher, jbyteArray sk, jbyteArray pk)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes pkBytes;
    auto pkBuffer = convertToInputBuffer(pkBytes, env, pk);
    ppc::crypto::BigNum decrypted_significant;
    int16_t exponent;
    bcos::bytes cipherCBytes;
    auto cipherBuffer = convertToInputBuffer(cipherCBytes, env, cipher);
    floating_point_paillier_decrypt_without_precompute(
        decrypted_significant.bn().get(), &exponent, &cipherBuffer, &skBuffer, &pkBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // convert to FloatingPointNumber
    jclass fpNumberClass = env->FindClass("com/webank/wedpr/sdk/jni/codec/FloatingPointNumber");
    // FloatingPointNumber(byte[] significantBytes, int exponent)
    jmethodID initFunID = env->GetMethodID(fpNumberClass, "<init>", "([BI)V");
    jbyteArray plainBytes = BigNumToJavaBigIntegerBytes(env, decrypted_significant.bn().get());
    jobject result = env->NewObject(fpNumberClass, initFunID, plainBytes, exponent);
    env->DeleteLocalRef(plainBytes);
    return result;
}

jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_impl(
    JNIEnv* env, jbyteArray c1, jbyteArray c2, jlong jpublic_key, bool add)
{
    auto pk = reinterpret_cast<void*>(jpublic_key);
    auto keyBits = paillier_key_bits_from_public_key(pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto cipherLen = floating_point_paillier_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};

    bcos::bytes c1CBytes;
    auto c1Buffer = convertToInputBuffer(c1CBytes, env, c1);
    bcos::bytes c2CBytes;
    auto c2Buffer = convertToInputBuffer(c2CBytes, env, c2);
    if (add)
    {
        floating_point_paillier_add(&cipherBuffer, &c1Buffer, &c2Buffer, pk);
    }
    else
    {
        floating_point_paillier_sub(&cipherBuffer, &c1Buffer, &c2Buffer, pk);
    }
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    add
 * Signature: ([B[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add(
    JNIEnv* env, jclass, jbyteArray c1, jbyteArray c2, jlong jpublic_key)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_impl(
        env, c1, c2, jpublic_key, true);
}


/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    sub
 * Signature: ([B[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_sub(
    JNIEnv* env, jclass, jbyteArray c1, jbyteArray c2, jlong jpublic_key)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_impl(
        env, c1, c2, jpublic_key, false);
}

jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_without_precompute_impl(
    JNIEnv* env, jbyteArray c1, jbyteArray c2, jbyteArray pkBytes, bool add)
{
    bcos::bytes pkCBytes;
    auto pkBuffer = convertToInputBuffer(pkCBytes, env, pkBytes);
    auto cipherLen = floating_point_paillier_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherBytes(cipherLen);
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    bcos::bytes c1Bytes;
    auto c1Buffer = convertToInputBuffer(c1Bytes, env, c1);
    bcos::bytes c2Bytes;
    auto c2Buffer = convertToInputBuffer(c2Bytes, env, c2);
    if (add)
    {
        floating_point_paillier_add_without_precompute(
            &cipherBuffer, &c1Buffer, &c2Buffer, &pkBuffer);
    }
    else
    {
        floating_point_paillier_sub_without_precompute(
            &cipherBuffer, &c1Buffer, &c2Buffer, &pkBuffer);
    }
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    decryptWithoutPrecompute
 * Signature: ([B[B[B)Lcom/webank/wedpr/sdk/jni/codec/FloatingPointNumber;
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_addWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray c1, jbyteArray c2, jbyteArray pkBytes)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_without_precompute_impl(
        env, c1, c2, pkBytes, true);
}
/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    addWithoutPrecompute
 * Signature: ([B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_subWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray c1, jbyteArray c2, jbyteArray pkBytes)
{
    return Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_add_without_precompute_impl(
        env, c1, c2, pkBytes, false);
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    scalaMul
 * Signature: ([BI[BJ)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_scalaMul(JNIEnv* env, jclass,
    jbyteArray significantBytes, jint exponent, jbyteArray cipherBytes, jlong jpublic_key)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);

    auto pk = reinterpret_cast<void*>(jpublic_key);
    auto keyBits = paillier_key_bits_from_public_key(pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto cipherLen = floating_point_paillier_cipher_bytes(keyBits);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherResult(cipherLen);
    OutputBuffer cipherResultBuffer{cipherResult.data(), cipherResult.size()};
    // get the cipher buffer
    bcos::bytes cipherCBytes;
    auto cipher = convertToInputBuffer(cipherCBytes, env, cipherBytes);

    floating_point_paillier_scalaMul(
        &cipherResultBuffer, significant.bn().get(), exponent, &cipher, pk);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(cipherResultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherResultBuffer.len, (jbyte*)cipherResultBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier
 * Method:    scalaMulWithoutPrecompute
 * Signature: ([BI[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingPointPaillier_scalaMulWithoutPrecompute(
    JNIEnv* env, jclass, jbyteArray significantBytes, jint exponent, jbyteArray cipherBytes,
    jbyteArray pkBytes)
{
    auto significant = JavaBigIntegerToBigNum(env, significantBytes);
    bcos::bytes pkCBytes;
    auto pkBuffer = convertToInputBuffer(pkCBytes, env, pkBytes);
    auto cipherLen = floating_point_paillier_cipher_bytes(pkBuffer.len * 8);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    bcos::bytes cipherResult(cipherLen);
    OutputBuffer cipherResultBuffer{cipherResult.data(), cipherResult.size()};
    // get the cipher buffer
    bcos::bytes cipherCBytes;
    auto cipher = convertToInputBuffer(cipherCBytes, env, cipherBytes);

    floating_point_paillier_scalaMul_without_precompute(
        &cipherResultBuffer, significant.bn().get(), exponent, &cipher, &pkBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(cipherResultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherResultBuffer.len, (jbyte*)cipherResultBuffer.data);
    return jResult;
}