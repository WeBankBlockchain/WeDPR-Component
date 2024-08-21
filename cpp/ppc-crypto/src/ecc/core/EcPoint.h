/**
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
 * @file EcPoint.h
 * @author: yujiechen
 * @date 2022-12-5
 */
#pragma once
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "../../Common.h"
#include "openssl/ec.h"
#include "openssl/obj_mac.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>

namespace ppc::crypto
{
struct EcGroupDeleter
{
public:
    void operator()(EC_GROUP* _group) { EC_GROUP_free(_group); }
};
using EcGroupPtr = std::shared_ptr<EC_GROUP>;

///// The ec-group
class EcGroup
{
public:
    EcGroup() = default;
    EcGroup(int _pointBytesLen, EC_GROUP* _group)
      : m_pointBytesLen(_pointBytesLen), m_ecGroup(EcGroupPtr(_group, EcGroupDeleter()))
    {
        auto bnContext = createBNContext();
        // obtain the curve params
        if (EC_GROUP_get_curve(m_ecGroup.get(), m_p.bn().get(), m_a.bn().get(), m_b.bn().get(),
                bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(ECGroupGetCurveError());
        }
        // get the order
        if (EC_GROUP_get_order(m_ecGroup.get(), m_n.bn().get(), bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(ECGroupGetOrderError());
        }
    }
    EcGroup(int _pointBytesLen, int _nid)
      : EcGroup(_pointBytesLen, EC_GROUP_new_by_curve_name(_nid))
    {}
    BigNum const& p() const { return m_p; }

    BigNum const& a() const { return m_a; }
    BigNum const& b() const { return m_b; }
    BigNum const& n() const { return m_n; }

    EcGroupPtr const& ecGroup() const { return m_ecGroup; }
    int pointBytesLen() const { return m_pointBytesLen; }

private:
    int m_pointBytesLen;
    BigNum m_p;
    BigNum m_a;
    BigNum m_b;

    BigNum m_n;
    EcGroupPtr m_ecGroup;
};

///// The ec-point
struct ECPointDeleter
{
public:
    void operator()(EC_POINT* point) { EC_POINT_clear_free(point); }
};
using ECPointPtr = std::shared_ptr<EC_POINT>;

class EcPoint
{
public:
    EcPoint(EcGroup _group)
      : m_group(std::move(_group)),
        m_point(ECPointPtr(EC_POINT_new(m_group.ecGroup().get()), ECPointDeleter()))
    {}
    EcPoint(EcGroup _group, bcos::bytesConstRef const& _data) : EcPoint(std::move(_group))
    {
        fromBytes(_data);
    }

    ~EcPoint() = default;

    // convert bytes to the point
    void fromBytes(bcos::bytesConstRef const& _data)
    {
        auto bnContext = createBNContext();
        BigNum pointNum;
        pointNum.fromBigEndianBytes((bcos::byte*)(_data.data()), _data.size(), false);
        auto bn = EC_POINT_bn2point(
            m_group.ecGroup().get(), pointNum.bn().get(), m_point.get(), bnContext.get());
        if (!bn)
        {
            BOOST_THROW_EXCEPTION(ECPointBn2PointError());
        }
    }

    // convert the point to bytes
    void toBytes(bcos::bytes& _data)
    {
        auto bnContext = createBNContext();
        unsigned char* buf = NULL;
        auto bufferLen = EC_POINT_point2buf(m_group.ecGroup().get(), m_point.get(),
            POINT_CONVERSION_COMPRESSED, &buf, bnContext.get());
        if (bufferLen == 0)
        {
            BOOST_THROW_EXCEPTION(ECPoint2BNError());
        }
        _data.resize(bufferLen);
        memcpy(_data.data(), buf, bufferLen);
        OPENSSL_free(buf);
    }

    // ecMultiply
    EcPoint ecMultiply(BigNum const& _sk) const
    {
        auto bnContext = createBNContext();
        EcPoint result(m_group);
        if (EC_POINT_mul(m_group.ecGroup().get(), result.point().get(), NULL, m_point.get(),
                _sk.bn().get(), bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(ECPointMulError());
        }
        return result;
    }

    EcPoint add(EcPoint const& _point) const
    {
        auto bnContext = createBNContext();
        EcPoint result(m_group);
        if (EC_POINT_add(m_group.ecGroup().get(), result.point().get(), m_point.get(),
                _point.point().get(), bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(ECPointAddError());
        }
        return result;
    }

    EcPoint sub(EcPoint& _point) const
    {
        auto bnContext = createBNContext();
        // invert the point
        if (EC_POINT_invert(m_group.ecGroup().get(), _point.point().get(), bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(
                ECPointSubError() << bcos::errinfo_comment("EcPoint sub error for invert failed"));
        }
        return add(_point);
    }

    EcGroup const& group() const { return m_group; }
    ECPointPtr const& point() const { return m_point; }

private:
    EcGroup m_group;
    ECPointPtr m_point;
};

// basePointMultiply
inline EcPoint basePointMultiply(EcGroup const& _group, BigNum const& _sk)
{
    auto bnContext = createBNContext();
    EcPoint result(_group);
    if (EC_POINT_mul(_group.ecGroup().get(), result.point().get(), _sk.bn().get(), NULL, NULL,
            bnContext.get()) != 1)
    {
        BOOST_THROW_EXCEPTION(ECPointMulError());
    }
    return result;
}

}  // namespace ppc::crypto