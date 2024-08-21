/**
 *  Copyright (C) 2023 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file JNIException.cpp
 * @author: yujiechen
 * @date 2023-08-14
 */
#include "JNIException.h"
#include <bcos-utilities/Exceptions.h>
#include <iostream>

void ThrowJNIException(JNIEnv* env, const char* kpFile, int iLine, const std::string& message)
{
    const char* className = "com/webank/wedpr/sdk/jni/common/JniException";
    (void)kpFile;
    (void)iLine;

    // Find the exception class.
    jclass tClass = env->FindClass(className);
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        tClass = env->FindClass("java/lang/Exception");
        if (tClass == NULL)
        {
            std::cerr << " Not found exception class: "
                      << "java/lang/Exception" << std::endl;
            env->DeleteLocalRef(tClass);
            return;
        }
    }

    // Throw the exception with error info
    env->ThrowNew(tClass, message.c_str());
    env->DeleteLocalRef(tClass);
}
