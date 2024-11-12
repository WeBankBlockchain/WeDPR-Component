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
 * @file TaskMock.h
 * @author: yujiechen
 * @date 2022-10-19
 */
#pragma once
#include "ppc-framework/protocol/Task.h"
#include <boost/test/unit_test.hpp>

using namespace ppc::protocol;
namespace ppc::test
{
inline PartyResource::Ptr mockParty(uint16_t _partyIndex, std::string const& _id,
    std::string const& _desc, std::string const& _dataId, ppc::protocol::DataResourceType _dataType,
    std::string const& _path, const std::vector<std::vector<std::string>>& _rowData = {})
{
    auto party = std::make_shared<PartyResource>(_id, (uint16_t)_partyIndex);
    party->setDesc(_desc);
    auto dataResource = std::make_shared<DataResource>(_dataId);
    auto dataDesc = std::make_shared<DataResourceDesc>();
    dataDesc->setType((uint16_t)_dataType);
    dataDesc->setPath(_path);
    dataResource->setDesc(dataDesc);
    party->setDataResource(dataResource);

    if (!_rowData.empty())
    {
        dataResource->setRawData(_rowData);
    }
    return party;
}

inline void checkParty(PartyResource::ConstPtr _party1, PartyResource::ConstPtr _party2)
{
    BOOST_CHECK(_party1->partyIndex() == _party2->partyIndex());
    BOOST_CHECK(_party1->desc() == _party2->desc());
    BOOST_CHECK(_party1->id() == _party2->id());
    BOOST_CHECK(_party1->partyIndex() == _party2->partyIndex());
    if (!_party1->dataResource())
    {
        return;
    }
    // check the dataResource
    BOOST_CHECK(_party1->dataResource()->resourceID() == _party2->dataResource()->resourceID());
    if (!_party1->dataResource()->desc())
    {
        return;
    }
    BOOST_CHECK(_party1->dataResource()->desc()->type() == _party2->dataResource()->desc()->type());
    // BOOST_CHECK(_party1->dataResource()->desc()->path() ==
    // _party2->dataResource()->desc()->path());
    BOOST_CHECK(_party1->dataResource()->desc()->accessCommand() ==
                _party2->dataResource()->desc()->accessCommand());

    BOOST_CHECK(
        _party1->dataResource()->rawData().size() == _party2->dataResource()->rawData().size());
}

inline void checkTask(Task::Ptr _task1, Task::Ptr _task2)
{
    BOOST_CHECK(_task1->id() == _task2->id());
    BOOST_CHECK(_task1->type() == _task2->type());
    BOOST_CHECK(_task1->param() == _task2->param());
    BOOST_CHECK(_task1->syncResultToPeer() == _task2->syncResultToPeer());
    // check self-party
    if (_task1->selfParty())
    {
        checkParty(_task1->selfParty(), _task2->selfParty());
    }
    // check peersParty
    auto const& peersParty = _task1->getAllPeerParties();
    BOOST_CHECK(peersParty.size() == _task2->getAllPeerParties().size());
    for (auto const& it : peersParty)
    {
        checkParty(it.second, _task2->getPartyByID(it.first));
    }
}
}  // namespace ppc::test
