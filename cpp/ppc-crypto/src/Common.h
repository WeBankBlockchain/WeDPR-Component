/*
 *  Copyright (C) 2022 WeDPR.
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
 * @date 2022-11-2
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Log.h>
#include <memory>

#define CRYPTO_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("CRYPTO")

namespace ppc::crypto
{
DERIVE_PPC_EXCEPTION(OprfFinalizeException);
DERIVE_PPC_EXCEPTION(HashToCurveException);
DERIVE_PPC_EXCEPTION(HashToScalarException);
DERIVE_PPC_EXCEPTION(ScalarInvertException);
DERIVE_PPC_EXCEPTION(ScalarCalculateException);
DERIVE_PPC_EXCEPTION(EcMultipleException);
DERIVE_PPC_EXCEPTION(MulGeneratorError);
DERIVE_PPC_EXCEPTION(EcAddError);
DERIVE_PPC_EXCEPTION(EcSubError);

DERIVE_PPC_EXCEPTION(ECGroupGetCurveError);
DERIVE_PPC_EXCEPTION(ECGroupGetOrderError);
DERIVE_PPC_EXCEPTION(ECPoint2BNError);
DERIVE_PPC_EXCEPTION(ECPointMulError);
DERIVE_PPC_EXCEPTION(ECPointAddError);
DERIVE_PPC_EXCEPTION(ECPointSubError);
DERIVE_PPC_EXCEPTION(ECPointBn2PointError);

DERIVE_PPC_EXCEPTION(UnsupportedCurveType);
DERIVE_PPC_EXCEPTION(GenerateRandomScalarError);

DERIVE_PPC_EXCEPTION(X25519GetSharedKeyError);
}  // namespace ppc::crypto
