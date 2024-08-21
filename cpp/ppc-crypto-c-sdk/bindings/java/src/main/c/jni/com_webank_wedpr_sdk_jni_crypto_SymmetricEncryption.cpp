#include "com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption.h"
#include "Common.h"
#include "JNIException.h"
#include "bcos-utilities/Common.h"
#include "ppc-crypto-c-sdk/symmetric_encryption.h"
#include "ppc-crypto-c-sdk/utils/error.h"
/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption
 * Method:    keyBytes
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption_keyBytes(
    JNIEnv* env, jclass, jint algorithmType, jint mode)
{
    auto keyBytes = symmetric_key_bytes(algorithmType, mode);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    return keyBytes;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption
 * Method:    generateKey
 * Signature: (II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption_generateKey(
    JNIEnv* env, jclass, jint algorithmType, jint mode)
{
    bcos::bytes keyData(symmetric_key_bytes(algorithmType, mode));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    OutputBuffer keyBuffer{keyData.data(), keyData.size()};
    symmetric_generate_key(&keyBuffer, algorithmType, mode);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    auto jResult = env->NewByteArray(keyBuffer.len);
    env->SetByteArrayRegion(jResult, 0, keyBuffer.len, (jbyte*)keyBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption
 * Method:    encrypt
 * Signature: (II[B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption_encrypt(
    JNIEnv* env, jclass, jint algorithmType, jint mode, jbyteArray sk, jbyteArray iv,
    jbyteArray plainData)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes ivBytes;
    auto ivBuffer = convertToInputBuffer(ivBytes, env, iv);
    bcos::bytes plainBytes;
    auto plainBuffer = convertToInputBuffer(plainBytes, env, plainData);
    bcos::bytes cipherData(symmetric_block_size(algorithmType) + plainBuffer.len);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};
    symmetric_encrypt(&cipherBuffer, algorithmType, mode, &skBuffer, &ivBuffer, &plainBuffer);
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
 * Class:     com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption
 * Method:    decrypt
 * Signature: (II[B[B[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_SymmetricEncryption_decrypt(
    JNIEnv* env, jclass, jint algorithm, jint mode, jbyteArray sk, jbyteArray iv, jbyteArray cipher)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes ivBytes;
    auto ivBuffer = convertToInputBuffer(ivBytes, env, iv);
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    bcos::bytes plainData(symmetric_block_size(algorithm) + cipherBuffer.len);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    OutputBuffer plainBuffer{plainData.data(), plainData.size()};
    symmetric_decrypt(&plainBuffer, algorithm, mode, &skBuffer, &ivBuffer, &cipherBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(plainBuffer.len);
    env->SetByteArrayRegion(jResult, 0, plainBuffer.len, (jbyte*)plainBuffer.data);
    return jResult;
}