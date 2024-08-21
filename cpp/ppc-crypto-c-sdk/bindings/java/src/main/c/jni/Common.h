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
 * @file Common.h
 * @author: yujiechen
 * @date 2023-08-14
 */

#pragma once

#include "openssl/bn.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <bcos-utilities/Common.h>
#include <jni.h>

// convert java bytes to InputBuffer
InputBuffer convertToInputBuffer(bcos::bytes& resultBuffer, JNIEnv* env, jbyteArray arrayData);
jbyteArray BigNumToJavaBigIntegerBytes(JNIEnv* env, BIGNUM* value);
// convert java big-integer to openssl BIGNUM
ppc::crypto::BigNum JavaBigIntegerToBigNum(JNIEnv* env, jbyteArray bigIntegerData);