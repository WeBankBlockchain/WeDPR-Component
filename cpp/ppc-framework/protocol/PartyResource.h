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
 * @file PartyResource.h
 * @author: yujiechen
 * @date 2022-10-13
 */
#pragma once
#include "DataResource.h"
#include "Protocol.h"
#include <bcos-utilities/Log.h>
#include <memory>
#include <sstream>
#include <vector>

namespace ppc::protocol
{
class PartyResource
{
public:
    using Ptr = std::shared_ptr<PartyResource>;
    using ConstPtr = std::shared_ptr<const PartyResource>;
    PartyResource() = default;
    PartyResource(std::string const& _id, uint16_t _partyIndex)
      : m_id(_id), m_partyIndex(_partyIndex)
    {}

    virtual ~PartyResource() = default;

    // the party index that represents the role in the task
    virtual uint16_t partyIndex() const { return m_partyIndex; }
    // the party id
    virtual std::string const& id() const { return m_id; }
    virtual std::string const& desc() const { return m_desc; }
    virtual DataResource::ConstPtr dataResource() const { return m_dataResource; }
    virtual DataResource::Ptr mutableDataResource() const { return m_dataResource; }

    virtual void setId(std::string const& _id) { m_id = _id; }
    virtual void setDesc(std::string _desc) { m_desc = _desc; }
    virtual void setDataResource(DataResource::Ptr _dataResource)
    {
        m_dataResource = _dataResource;
    }
    // the mpc index
    virtual void setPartyIndex(uint16_t _partyIndex) { m_partyIndex = _partyIndex; }

private:
    std::string m_id;
    uint16_t m_partyIndex;
    std::string m_desc;
    DataResource::Ptr m_dataResource;
};
using Parties = std::vector<PartyResource::Ptr>;
using ConstParties = std::vector<PartyResource::ConstPtr>;

inline std::string printPartyInfo(PartyResource::ConstPtr _party)
{
    if (!_party)
    {
        return "empty";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("partyId", _party->id()) << LOG_KV("partyIndex", _party->partyIndex())
                 << LOG_KV("desc", _party->desc());

    if (_party->dataResource())
    {
        auto const& dataResource = _party->dataResource();
        stringstream << printDataResourceInfo(dataResource);
    }
    return stringstream.str();
}
}  // namespace ppc::protocol
