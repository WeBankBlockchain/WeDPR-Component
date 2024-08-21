#include "com_webank_wedpr_sdk_jni_crypto_FastOre.h"
#include "Common.h"
#include "JNIException.h"
#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include <bcos-utilities/Common.h>

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    keyBytes
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_keyBytes(JNIEnv* env, jclass)
{
    auto keyBytes = fast_ore_key_bytes();
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    return keyBytes;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    cipherSize
 * Signature: (JZ)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_cipherSize(
    JNIEnv* env, jclass, jlong plainSize, jboolean hex)
{
    auto cipherSize = fast_ore_get_cipher_size(plainSize, hex);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    return cipherSize;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    plainSize
 * Signature: (JZ)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_plainSize(
    JNIEnv* env, jclass, jlong cipherSize, jboolean hex)
{
    auto plainSize = fast_ore_get_plain_size(cipherSize, hex);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    return plainSize;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    generateKey
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_generateKey(
    JNIEnv* env, jclass cls)
{
    auto keyBytes = Java_com_webank_wedpr_sdk_jni_crypto_FastOre_keyBytes(env, cls);
    bcos::bytes keyData(keyBytes);
    OutputBuffer keyBuffer{keyData.data(), keyData.size()};
    fast_ore_generate_key(&keyBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(keyBuffer.len);
    env->SetByteArrayRegion(jResult, 0, keyBuffer.len, (jbyte*)keyBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    encrypt4String
 * Signature: ([B[BZ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_encrypt4String(
    JNIEnv* env, jclass cls, jbyteArray sk, jbyteArray plainData, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes plainBytes;
    auto plainBuffer = convertToInputBuffer(plainBytes, env, plainData);
    bcos::bytes cipherBytes(
        Java_com_webank_wedpr_sdk_jni_crypto_FastOre_cipherSize(env, cls, plainBuffer.len, hex));
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    string_fast_ore_encrypt(&cipherBuffer, &skBuffer, &plainBuffer, hex);

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
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    decrypt4String
 * Signature: ([B[BZ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_decrypt4String(
    JNIEnv* env, jclass cls, jbyteArray sk, jbyteArray cipher, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    bcos::bytes plainBytes(
        Java_com_webank_wedpr_sdk_jni_crypto_FastOre_plainSize(env, cls, cipherBuffer.len, hex));
    OutputBuffer plainBuffer{plainBytes.data(), plainBytes.size()};
    string_fast_ore_decrypt(&plainBuffer, &skBuffer, &cipherBuffer, hex);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(plainBuffer.len);
    env->SetByteArrayRegion(jResult, 0, plainBuffer.len, (jbyte*)plainBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    encrypt4Integer
 * Signature: ([BJZ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_encrypt4Integer(
    JNIEnv* env, jclass cls, jbyteArray sk, jlong plain, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes cipherBytes(
        Java_com_webank_wedpr_sdk_jni_crypto_FastOre_cipherSize(env, cls, sizeof(plain), hex));
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    integer_fast_ore_encrypt(&cipherBuffer, &skBuffer, static_cast<int64_t>(plain), hex);
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
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    decrypt4Integer
 * Signature: ([B[BZ)J
 */
JNIEXPORT jlong JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_decrypt4Integer(
    JNIEnv* env, jclass cls, jbyteArray sk, jbyteArray cipher, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    int64_t plain = 0;
    integer_fast_ore_decrypt(&plain, &skBuffer, &cipherBuffer, hex);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return 0;
    }
    return plain;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    encrypt4Float
 * Signature: ([B[BZ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_encrypt4Float(
    JNIEnv* env, jclass, jbyteArray sk, jbyteArray plainData, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes plainBytes;
    auto plainBuffer = convertToInputBuffer(plainBytes, env, plainData);

    bcos::bytes cipherBytes(fast_ore_get_float_cipher_size(plainBuffer.len, hex));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    float_fast_ore_encrypt(&cipherBuffer, &skBuffer, &plainBuffer, hex);
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
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    decrypt4Float
 * Signature: ([B[BZ)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_decrypt4Float(
    JNIEnv* env, jclass, jbyteArray sk, jbyteArray cipherData, jboolean hex)
{
    bcos::bytes skBytes;
    auto skBuffer = convertToInputBuffer(skBytes, env, sk);
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipherData);

    bcos::bytes plainBytes(64);
    OutputBuffer plainBuffer{plainBytes.data(), plainBytes.size()};
    float_fast_ore_decrypt(&plainBuffer, &skBuffer, &cipherBuffer, hex);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(plainBuffer.len);
    env->SetByteArrayRegion(jResult, 0, plainBuffer.len, (jbyte*)plainBuffer.data);
    return jResult;
}

/*
 * Class:     com_webank_wedpr_sdk_jni_crypto_FastOre
 * Method:    compare
 * Signature: ([B[B)I
 */
JNIEXPORT jint JNICALL Java_com_webank_wedpr_sdk_jni_crypto_FastOre_compare(
    JNIEnv* env, jclass, jbyteArray c1, jbyteArray c2)
{
    bcos::bytes c1Bytes;
    auto c1Buffer = convertToInputBuffer(c1Bytes, env, c1);
    bcos::bytes c2Bytes;
    auto c2Buffer = convertToInputBuffer(c2Bytes, env, c2);
    auto ret = fast_ore_compare(&c1Buffer, &c2Buffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return -1;
    }
    return ret;
}
