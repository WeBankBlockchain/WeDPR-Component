#include "com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc.h"
#include "Common.h"
#include "JNIException.h"
#include "ppc-crypto-c-sdk/floating_point_ihc.h"
#include "ppc-crypto-c-sdk/homo_ihc.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include <bcos-utilities/Common.h>
#include <cstdio>

JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_generateKey(
    JNIEnv* env, jclass, jint mode)
{
    bcos::bytes keyBytes(ihc_key_bytes(mode));
    OutputBuffer key{keyBytes.data(), keyBytes.size()};
    ihc_generate_key(&key, mode);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
    }
    auto jResult = env->NewByteArray(key.len);
    env->SetByteArrayRegion(jResult, 0, key.len, (jbyte*)key.data);
    return jResult;
}

JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_encrypt(
    JNIEnv* env, jclass, jint mode, jbyteArray key, jbyteArray significant, jint exponent)
{
    bcos::bytes cipherBytes(ihc_floating_cipher_bytes(mode));
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    auto bigNumValue = JavaBigIntegerToBigNum(env, significant);

    bcos::bytes keyBytes;
    auto keyBuffer = convertToInputBuffer(keyBytes, env, key);
    ihc_floating_encrypt(&cipherBuffer, mode, &keyBuffer, bigNumValue.bn().get(), exponent);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}

JNIEXPORT jobject JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_decrypt(
    JNIEnv* env, jclass, jint mode, jbyteArray key, jbyteArray cipher)
{
    bcos::bytes cipherCBytes;
    auto cipherBuffer = convertToInputBuffer(cipherCBytes, env, cipher);
    bcos::bytes keyBytes;
    auto keyBuffer = convertToInputBuffer(keyBytes, env, key);

    ppc::FloatingPointNumber resultFp;
    ihc_floating_decrypt(
        resultFp.value.bn().get(), &resultFp.exponent, mode, &keyBuffer, &cipherBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    // convert to FloatingPointNumber
    jclass fpNumberClass = env->FindClass("com/webank/wedpr/sdk/jni/codec/FloatingPointNumber");
    // FloatingPointNumber(byte[] significantBytes, int exponent)
    jmethodID initFunID = env->GetMethodID(fpNumberClass, "<init>", "([BI)V");
    jbyteArray cipherBytes = BigNumToJavaBigIntegerBytes(env, resultFp.value.bn().get());
    jobject result = env->NewObject(fpNumberClass, initFunID, cipherBytes, resultFp.exponent);
    // release the used jbyteArray
    env->DeleteLocalRef(cipherBytes);
    return result;
}


JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_add(
    JNIEnv* env, jclass, jint mode, jbyteArray cipher1, jbyteArray cipher2)
{
    bcos::bytes cipherBytes1;
    auto cipherBuffer1 = convertToInputBuffer(cipherBytes1, env, cipher1);
    bcos::bytes cipherBytes2;
    auto cipherBuffer2 = convertToInputBuffer(cipherBytes2, env, cipher2);
    bcos::bytes cipherBytes(ihc_floating_cipher_bytes(mode));
    OutputBuffer resultBuffer{cipherBytes.data(), cipherBytes.size()};
    ihc_floating_add(&resultBuffer, mode, &cipherBuffer1, &cipherBuffer2);

    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}


JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_sub(
    JNIEnv* env, jclass, jint mode, jbyteArray cipher1, jbyteArray cipher2)
{
    bcos::bytes cipherBytes1;
    auto cipherBuffer1 = convertToInputBuffer(cipherBytes1, env, cipher1);
    bcos::bytes cipherBytes2;
    auto cipherBuffer2 = convertToInputBuffer(cipherBytes2, env, cipher2);
    bcos::bytes cipherBytes(ihc_floating_cipher_bytes(mode));
    OutputBuffer cipherBuffer{cipherBytes.data(), cipherBytes.size()};
    ihc_floating_sub(&cipherBuffer, mode, &cipherBuffer1, &cipherBuffer2);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }

    auto jResult = env->NewByteArray(cipherBuffer.len);
    env->SetByteArrayRegion(jResult, 0, cipherBuffer.len, (jbyte*)cipherBuffer.data);
    return jResult;
}


JNIEXPORT jbyteArray JNICALL Java_com_webank_wedpr_sdk_jni_homo_NativeFloatingIhc_scalaMul(
    JNIEnv* env, jclass, jint mode, jbyteArray vSignificant, jint vExponent, jbyteArray cipher)
{
    bcos::bytes cipherBytes;
    auto cipherBuffer = convertToInputBuffer(cipherBytes, env, cipher);
    auto bigNumValue = JavaBigIntegerToBigNum(env, vSignificant);
    bcos::bytes resultBytes(ihc_floating_cipher_bytes(mode));
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    ihc_floating_scalaMul(&resultBuffer, mode, bigNumValue.bn().get(), vExponent, &cipherBuffer);
    if (!last_call_success())
    {
        THROW_JNI_EXCEPTION(env, get_last_error_msg());
        return nullptr;
    }
    auto jResult = env->NewByteArray(resultBuffer.len);
    env->SetByteArrayRegion(jResult, 0, resultBuffer.len, (jbyte*)resultBuffer.data);
    return jResult;
}