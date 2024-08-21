/**
 *  Copyright (C) 2021 WeDPR.
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
 * @file SimplestOT.cpp
 * @author: shawnhe
 * @date 2022-07-06
 */

#include "SimplestOT.h"
#include <algorithm>

using namespace ppc::crypto;
using namespace ppc::protocol;

std::pair<bcos::bytesPointer, std::vector<bcos::bytes>> SimplestOT::receiverGeneratePointsB(
    BitVector::Ptr _choices, bcos::bytesPointer _pointA)
{
    auto otNumber = _choices->size();
    bcos::bytesPointer bytesB = std::make_shared<bcos::bytes>(otNumber * m_ecc->pointSizeInBytes());
    std::vector<bcos::bytes> scalars(otNumber);

    for (uint32_t i = 0; i < otNumber; ++i)
    {
        scalars[i] = m_ecc->generateRandomScalar();
        std::array<bcos::bytes, 2> pairB;
        pairB[0] = m_ecc->mulGenerator(scalars[i]);
        pairB[1] = m_ecc->ecAdd(*_pointA, pairB[0]);
        memcpy(bytesB->data() + i * m_ecc->pointSizeInBytes(), pairB[_choices->get(i)].data(),
            m_ecc->pointSizeInBytes());
    }

    return {bytesB, scalars};
}

std::vector<bcos::bytes> SimplestOT::finishReceiver(
    bcos::bytesPointer _pointA, std::vector<bcos::bytes> _bScalars)
{
    auto otNumber = _bScalars.size();
    std::vector<bcos::bytes> keys(otNumber);

    for (uint32_t i = 0; i < otNumber; ++i)
    {
        auto point = m_ecc->ecMultiply(*_pointA, _bScalars[i]);
        auto state = m_hash->init();
        m_hash->update(state, bcos::ref(point));
        bcos::bytes indexData((bcos::byte*)(&i), (bcos::byte*)(&i) + sizeof(uint32_t));
        m_hash->update(state, bcos::ref(indexData));
        keys[i] = m_hash->final(state);
    }
    return keys;
}

std::pair<bcos::bytes, bcos::bytesPointer> SimplestOT::senderGeneratePointA()
{
    auto a = m_ecc->generateRandomScalar();
    auto A = m_ecc->mulGenerator(a);
    return {a, std::make_shared<bcos::bytes>(A)};
}

std::vector<std::array<bcos::bytes, 2>> SimplestOT::finishSender(
    bcos::bytes _aScalar, bcos::bytesPointer _pointA, bcos::bytesPointer _pointsB)
{
    uint32_t otNumber = _pointsB->size() / m_ecc->pointSizeInBytes();
    std::vector<std::array<bcos::bytes, 2>> keys(otNumber);

    auto A = m_ecc->ecMultiply(*_pointA, _aScalar);

    for (uint32_t i = 0; i < otNumber; ++i)
    {
        bcos::bytes indexData((bcos::byte*)(&i), (bcos::byte*)(&i) + sizeof(uint32_t));

        bcos::bytes B(_pointsB->begin() + i * m_ecc->pointSizeInBytes(),
            _pointsB->begin() + (i + 1) * m_ecc->pointSizeInBytes());
        auto point0 = m_ecc->ecMultiply(B, _aScalar);
        auto state0 = m_hash->init();
        m_hash->update(state0, bcos::ref(point0));
        m_hash->update(state0, bcos::ref(indexData));
        keys[i][0] = m_hash->final(state0);

        auto point1 = m_ecc->ecSub(point0, A);
        auto state1 = m_hash->init();
        m_hash->update(state1, bcos::ref(point1));
        m_hash->update(state1, bcos::ref(indexData));
        keys[i][1] = m_hash->final(state1);
    }

    return keys;
}
