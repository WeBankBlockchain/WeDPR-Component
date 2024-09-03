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
 * @file JsonTaskImpl.h
 * @author: yujiechen
 * @date 2022-10-18
 */
#pragma once
#include "ppc-framework/protocol/Task.h"
#include <bcos-utilities/RefDataContainer.h>
#include <json/json.h>

#include <utility>

namespace ppc::protocol
{
class JsonTaskImpl : public Task
{
public:
    using Ptr = std::shared_ptr<JsonTaskImpl>;
    JsonTaskImpl(std::string const& _selfPartyID, std::string const& _prePath = "data")
      : m_selfPartyID(_selfPartyID), m_prePath(_prePath)
    {}
    JsonTaskImpl(std::string const& _selfPartyID, std::string_view _taskData,
        std::string const& _prePath = "data")
      : JsonTaskImpl(_selfPartyID, _prePath)
    {
        decode(_taskData);
    }

    JsonTaskImpl(std::string const& _selfPartyID, Json::Value const& _taskJson,
        std::string const& _prePath = "data")
      : JsonTaskImpl(_selfPartyID, _prePath)
    {
        decodeJsonValue(_taskJson);
    }

    ~JsonTaskImpl() override = default;

    // the taskID
    std::string const& id() const override { return m_id; }
    // the taskType, e.g. PSI
    uint8_t type() const override { return m_type; }
    // the task algorithm, e.g.CM_PSI_2PC/RA_PSI_2PC
    uint8_t algorithm() const override { return m_algorithm; }

    // information of the local party
    PartyResource::ConstPtr selfParty() const override { return m_self; }
    PartyResource::Ptr mutableSelfParty() const override { return m_self; }

    // information of the peer party(not thread-safe)
    PartyResource::ConstPtr getPartyByID(std::string const& _partyID) const override;
    PartyResource::ConstPtr getPartyByIndex(uint16_t _partyIndex) const override;
    // get all peer-parties(not thread-safe)
    std::map<std::string, PartyResource::Ptr> const& getAllPeerParties() const override
    {
        return m_peerParties;
    }

    std::vector<std::string> const& getReceiverLists() const override { return m_receiverLists; }

    // params of the task, can be deserialized using json
    std::string const& param() const override { return m_param; }

    // sync the psi result to peer or not
    bool syncResultToPeer() const override { return m_syncResultToPeer; }
    bool lowBandwidth() const override { return m_lowBandwidth; }

    void setId(std::string const& _id) override { m_id = _id; }
    void setType(uint8_t _type) override { m_type = _type; }
    void setAlgorithm(uint8_t _algorithm) override { m_algorithm = _algorithm; }
    void setSelf(PartyResource::Ptr _self) override { m_self = _self; }

    // Note: not thread-safe here
    void addParty(PartyResource::Ptr _party) override { m_peerParties[_party->id()] = _party; }
    void setParam(std::string const& _param) override { m_param = _param; }

    void setSyncResultToPeer(bool _syncResultToPeer) override
    {
        m_syncResultToPeer = _syncResultToPeer;
    }
    void setLowBandwidth(bool _lowBandwidth) override { m_lowBandwidth = _lowBandwidth; }

    // decode the task
    void decode(std::string_view _taskData) override;
    virtual void decodeJsonValue(Json::Value const& root);
    // encode the task into json
    std::string encode() const override;

public:
    void decodePartyInfo(PartyResource::Ptr _partyInfo, Json::Value const& _root);
    DataResource::Ptr decodeDataResource(Json::Value const& _root);
    void decodeDataResourceDesc(DataResourceDesc::Ptr _desc, Json::Value const& _value);

    void encodePartyInfo(Json::Value& _value, PartyResource::Ptr _party) const;
    void encodeDataResource(Json::Value& _value, DataResource::ConstPtr _dataResource) const;
    Json::Value encodeDataResourceDesc(DataResourceDesc::ConstPtr _desc) const;

    void checkNonEmptyField(std::string const& _field, std::string const& _value)
    {
        if (_value.empty())
        {
            BOOST_THROW_EXCEPTION(bcos::InvalidParameter() << bcos::errinfo_comment(
                                      "The value of " + _field + " must be non-empty"));
        }
    }

    void checkNull(Json::Value const& _value, std::string const& _field)
    {
        if (!_value.isNull())
        {
            return;
        }
        BOOST_THROW_EXCEPTION(bcos::InvalidParameter() << bcos::errinfo_comment(
                                  "The value of " + _field + " must be not null!"));
    }

private:
    std::string m_selfPartyID;
    std::string m_prePath;
    std::string m_id;
    // the task type
    uint8_t m_type;
    // the task-algorithm
    uint8_t m_algorithm;
    PartyResource::Ptr m_self;
    // the partyID to PartyResource
    std::map<std::string, PartyResource::Ptr> m_peerParties;
    std::vector<std::string> m_receiverLists;
    // parameters
    std::string m_param;
    bool m_syncResultToPeer = false;
    bool m_lowBandwidth = false;
};

class JsonTaskFactory : public TaskFactory
{
public:
    using Ptr = std::shared_ptr<JsonTaskFactory>;
    using ConstPtr = std::shared_ptr<JsonTaskFactory const>;
    JsonTaskFactory(std::string const& _selfPartyID, const std::string& _prePath = "data")
      : m_selfPartyID(_selfPartyID), m_prePath(_prePath)
    {}
    ~JsonTaskFactory() override = default;

    Task::Ptr createTask(std::string_view _taskInfo) override
    {
        return std::make_shared<JsonTaskImpl>(m_selfPartyID, _taskInfo, m_prePath);
    }

    virtual Task::Ptr createTask(Json::Value const& _taskInfo)
    {
        return std::make_shared<JsonTaskImpl>(m_selfPartyID, _taskInfo, m_prePath);
    }

    Task::Ptr createTask() override
    {
        return std::make_shared<JsonTaskImpl>(m_selfPartyID, m_prePath);
    }

private:
    std::string m_selfPartyID;
    std::string m_prePath;
};
}  // namespace ppc::protocol