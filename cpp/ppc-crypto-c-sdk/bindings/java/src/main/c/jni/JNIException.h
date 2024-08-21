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
 * @file JNIException.h
 * @author: yujiechen
 * @date 2023-08-14
 */
#include <jni.h>
#include <string>
/* Header for class common */

#ifndef __JNI_EXCEPTION_H__
#define __JNI_EXCEPTION_H__

#ifdef __cplusplus
extern "C" {
#endif

#define THROW_JNI_EXCEPTION(_ENV_, _INFO_) ThrowJNIException(_ENV_, __FILE__, __LINE__, _INFO_);

void ThrowJNIException(JNIEnv* env, const char* kpFile, int iLine, const std::string& message);

#ifdef __cplusplus
}
#endif
#endif