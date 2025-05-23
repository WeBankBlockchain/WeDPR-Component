/*
 *  Copyright (C) 2024 WeDPR.
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
 * @file RouterTableImpl.h
 * @author: yujiechen
 * @date 2022-5-24
 */
#pragma once
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "RouterTableInterface.h"
#include "tars/RouterTable.h"
#include <wedpr-protocol/tars/Common.h>
#include <memory>

namespace ppc::gateway
{
class RouterTableEntry : public RouterTableEntryInterface
{
public:
    using Ptr = std::shared_ptr<RouterTableEntry>;
    RouterTableEntry()
      : m_inner([m_entry = ppctars::RouterTableEntry()]() mutable { return &m_entry; })
    {}
    RouterTableEntry(std::function<ppctars::RouterTableEntry*()> _inner)
      : m_inner(std::move(_inner))
    {}
    RouterTableEntry(RouterTableEntry&&) = delete;
    RouterTableEntry(const RouterTableEntry&) = delete;
    RouterTableEntry& operator=(const RouterTableEntry&) = delete;
    RouterTableEntry& operator=(RouterTableEntry&&) = delete;
    ~RouterTableEntry() override = default;

    void setDstNode(std::string const& _dstNode) override { m_inner()->dstNode = _dstNode; }
    void setNextHop(std::string const& _nextHop) override { m_inner()->nextHop = _nextHop; }
    void clearNextHop() override { m_inner()->nextHop = std::string(); }
    void setDistance(int32_t _distance) override { m_inner()->distance = _distance; }
    void incDistance(int32_t _deltaDistance) override { m_inner()->distance += _deltaDistance; }

    std::string const& dstNode() const override { return m_inner()->dstNode; }
    std::string const& nextHop() const override { return m_inner()->nextHop; }
    int32_t distance() const override { return m_inner()->distance; }

    ppctars::RouterTableEntry const& inner() const { return *(m_inner()); }

private:
    std::function<ppctars::RouterTableEntry*()> m_inner;
};

class RouterTable : public RouterTableInterface
{
public:
    using Ptr = std::shared_ptr<RouterTable>;
    RouterTable() : m_inner([m_table = ppctars::RouterTable()]() mutable { return &m_table; }) {}
    RouterTable(bcos::bytesConstRef _decodedData) : RouterTable() { decode(_decodedData); }
    RouterTable(RouterTable&&) = delete;
    RouterTable(const RouterTable&) = delete;
    RouterTable& operator=(const RouterTable&) = delete;
    RouterTable& operator=(RouterTable&&) = delete;
    ~RouterTable() override = default;

    void encode(bcos::bytes& _encodedData) override;
    void decode(bcos::bytesConstRef _decodedData) override;

    std::map<std::string, RouterTableEntryInterface::Ptr> const& routerEntries() override
    {
        return m_routerEntries;
    }
    // append the unreachableNodes into param _unreachableNodes
    bool update(std::set<std::string>& _unreachableNodes, std::string const& _generatedFrom,
        RouterTableEntryInterface::Ptr _entry) override;
    // append the unreachableNodes into param _unreachableNodes
    bool erase(std::set<std::string>& _unreachableNodes, std::string const& _p2pNodeID) override;

    void setNodeID(std::string const& _nodeID) override { m_nodeID = _nodeID; }
    std::string const& nodeID() const override { return m_nodeID; }

    void setUnreachableDistance(int _unreachableDistance) override
    {
        m_unreachableDistance = _unreachableDistance;
    }

    int unreachableDistance() const override { return m_unreachableDistance; }
    std::string getNextHop(std::string const& _nodeID) override;
    std::set<std::string> getAllReachableNode() override;

    bool updateDstNodeEntry(
        std::string const& _generatedFrom, RouterTableEntryInterface::Ptr _entry);
    void updateDistanceForAllRouterEntries(std::set<std::string>& _unreachableNodes,
        std::string const& _nextHop, int32_t _newDistance);

private:
    std::string m_nodeID;
    std::function<ppctars::RouterTable*()> m_inner;
    std::map<std::string, RouterTableEntryInterface::Ptr> m_routerEntries;
    mutable bcos::SharedMutex x_routerEntries;

    int m_unreachableDistance = 10;
};

class RouterTableFactoryImpl : public RouterTableFactory
{
public:
    using Ptr = std::shared_ptr<RouterTableFactoryImpl>;
    RouterTableInterface::Ptr createRouterTable() override
    {
        return std::make_shared<RouterTable>();
    }
    RouterTableInterface::Ptr createRouterTable(bcos::bytesConstRef _decodedData) override
    {
        return std::make_shared<RouterTable>(_decodedData);
    }

    RouterTableEntryInterface::Ptr createRouterEntry() override
    {
        return std::make_shared<RouterTableEntry>();
    }
};
}  // namespace ppc::gateway