/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file RA2018MessageFixture.h
 * @author: yujiechen
 * @date 2022-11-16
 */
#pragma once
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-psi/src/ra2018-psi/protocol/RA2018Message.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::tools;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
inline CuckooFilterInfo::Ptr fakeCuckooFilterInfo(
    int32_t _filterID, Hash::Ptr hashImpl, CuckoofilterOption::Ptr option, uint64_t insertedKeySize)
{
    auto filterInfo = std::make_shared<CuckooFilterInfo>(_filterID);
    auto filter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(option);
    std::vector<bcos::bytes> hashList;
    for (uint64_t i = 0; i < insertedKeySize; i++)
    {
        std::string input = std::to_string(i);
        bcos::bytes inputData(input.begin(), input.end());
        hashList.emplace_back(hashImpl->hash(ref(inputData)));
    }
    // insert the hashList and check
    auto tmpHashList = hashList;
    auto result = filter->batchInsert(tmpHashList);
    BOOST_CHECK(tmpHashList.empty() == true);
    BOOST_CHECK(result);
    // set the hash
    auto md5Hash = std::make_shared<MD5Hash>();
    auto encodedData = filter->serialize();
    auto hashResult =
        md5Hash->hash(bytesConstRef((bcos::byte*)encodedData.data(), encodedData.size()));
    filterInfo->setHash(hashResult);
    // set the cuckoo-filter
    filterInfo->setCuckooFilter(std::move(filter));
    return filterInfo;
}

inline void checkMessage(PSIMessageInterface::Ptr _msg, uint32_t _packetType,
    std::string const& _partyID, std::string const& _resourceID, int32_t _version,
    std::vector<bcos::bytes> const& _data)
{
    // check the message
    BOOST_CHECK(_msg->partyID() == _partyID);
    BOOST_CHECK(_msg->resourceID() == _resourceID);
    BOOST_CHECK(_msg->version() == _version);
    BOOST_CHECK(_data.size() == _msg->dataSize());
    uint64_t i = 0;
    for (uint64_t i = 0; i < _msg->dataSize(); i++)
    {
        auto msgData = _msg->getData(i);
        BOOST_CHECK(_data.at(i) == bytes(msgData.begin(), msgData.end()));
    }
}

inline void fakeAndCheckRA2018Message(PSIMessageInterface::Ptr msg,
    RA2018MessageFactory::Ptr _msgFactory, uint32_t _packetType, std::string const& _partyID,
    std::string const& _resourceID, int32_t _version, std::vector<bcos::bytes> const& _data)
{
    msg->setPacketType(_packetType);
    msg->setPartyID(_partyID);
    msg->setResourceID(_resourceID);
    msg->setVersion(_version);
    auto tmpData = _data;
    msg->setData(std::move(tmpData));

    // check the result
    checkMessage(msg, _packetType, _partyID, _resourceID, _version, _data);
    // encode
    auto encodedData = msg->encode();
    // decode
    auto decodedMsg = _msgFactory->decodePSIMessage(ref(*encodedData));
    checkMessage(decodedMsg, _packetType, _partyID, _resourceID, _version, _data);

    // check takeData
    auto takedData = decodedMsg->takeData();
    uint64_t i = 0;
    for (auto const& it : takedData)
    {
        BOOST_CHECK(_data.at(i) == it);
        i++;
    }
}

inline void checkCuckooFilter(
    RA2018FilterMessage::Ptr _msg, std::vector<CuckooFilterInfo::Ptr> _cuckooFilters)
{
    auto& msgCuckooFilters = _msg->mutableFilterInfo();
    BOOST_CHECK(msgCuckooFilters.size() == _cuckooFilters.size());
    auto md5Hash = std::make_shared<MD5Hash>();
    for (uint64_t i = 0; i < msgCuckooFilters.size(); i++)
    {
        auto msgCuckooFilter = msgCuckooFilters.at(i);
        auto cuckooFilter = _cuckooFilters.at(i);
        BOOST_CHECK(msgCuckooFilter->filterID() == cuckooFilter->filterID());
        BOOST_CHECK(msgCuckooFilter->hash() == cuckooFilter->hash());
        if (cuckooFilter->cuckooFilter())
        {
            BOOST_CHECK(msgCuckooFilter->cuckooFilter() != nullptr);
            auto cuckooFilterEncodedData = cuckooFilter->cuckooFilter()->serialize();
            BOOST_CHECK(msgCuckooFilter->cuckooFilter()->serialize() == cuckooFilterEncodedData);
            // check the hash
            auto hashResult = md5Hash->hash(bytesConstRef(
                (bcos::byte*)(cuckooFilterEncodedData.data()), cuckooFilterEncodedData.size()));
            BOOST_CHECK(hashResult == cuckooFilter->hash());
        }
        BOOST_CHECK(msgCuckooFilter->cuckooFilterData() == cuckooFilter->cuckooFilterData());
    }
}

inline void fakeAndCheckRA2018FilterMessage(PSIMessageInterface::Ptr _msg,
    RA2018MessageFactory::Ptr _msgFactory, uint32_t _packetType, std::string const& _partyID,
    std::string const& _resourceID, int32_t _version, std::vector<bcos::bytes> const& _data,
    std::vector<CuckooFilterInfo::Ptr> _cuckooFilters)
{
    auto msg = std::dynamic_pointer_cast<RA2018FilterMessage>(_msg);
    // with empty cuckoo-filter
    fakeAndCheckRA2018Message(
        msg, _msgFactory, _packetType, _partyID, _resourceID, _version, _data);

    // set the cuckoo-filter
    auto cuckooFilters = _cuckooFilters;
    msg->setFilterInfo(std::move(_cuckooFilters));
    // check the cuckooFilters
    checkCuckooFilter(msg, cuckooFilters);

    // encode
    auto encodedData = msg->encode();
    auto decodedMsg = _msgFactory->decodePSIMessage(ref(*encodedData));
    checkMessage(decodedMsg, _packetType, _partyID, _resourceID, _version, _data);
    auto decodedFilterMsg = std::dynamic_pointer_cast<RA2018FilterMessage>(decodedMsg);
    // check the cuckoo-filters
    checkCuckooFilter(decodedFilterMsg, cuckooFilters);
}

inline void fakeAndCheckNotificationMessage(PSIMessageInterface::Ptr _msg,
    RA2018MessageFactory::Ptr _msgFactory, uint32_t _packetType, std::string const& _partyID,
    std::string const& _resourceID, int32_t _version, std::vector<bcos::bytes> const& _data,
    int32_t _errorCode, std::string const& _errorMsg)
{
    auto msg = std::dynamic_pointer_cast<PSITaskNotificationMessage>(_msg);
    // with default errorCode/errorMessage
    fakeAndCheckRA2018Message(
        msg, _msgFactory, _packetType, _partyID, _resourceID, _version, _data);
    msg->setErrorCode(_errorCode);
    msg->setErrorMessage(_errorMsg);

    BOOST_CHECK(msg->errorCode() == _errorCode);
    BOOST_CHECK(msg->errorMessage() == _errorMsg);
    // encode
    auto encodedData = msg->encode();
    // decode
    auto decodedMsg = _msgFactory->decodePSIMessage(ref(*encodedData));
    auto decodedNotificationMsg = std::dynamic_pointer_cast<PSITaskNotificationMessage>(decodedMsg);
    // check the errorCode/errorMessage
    BOOST_CHECK(decodedNotificationMsg->errorCode() == _errorCode);
    BOOST_CHECK(decodedNotificationMsg->errorMessage() == _errorMsg);
}

}  // namespace ppc::test