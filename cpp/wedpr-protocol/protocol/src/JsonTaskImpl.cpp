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
 * @file JsonTaskImpl.cpp
 * @author: yujiechen
 * @date 2022-10-18
 */
#include "JsonTaskImpl.h"
#include <bcos-utilities/Exceptions.h>
#include <boost/filesystem.hpp>

using namespace ppc;
using namespace bcos;
using namespace ppc::protocol;


void JsonTaskImpl::decode(std::string_view _taskData)
{
    Json::Value root;
    Json::Reader jsonReader;

    if (!jsonReader.parse(std::string(_taskData), root))
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
                                  "The task information must be valid json string."));
    }
    decodeJsonValue(root);
}

void JsonTaskImpl::decodeJsonValue(Json::Value const& root)
{
    // the taskID
    if (!root.isMember("taskID"))
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment("Must specify the taskID"));
    }
    checkNull(root["taskID"], "taskID");
    m_id = root["taskID"].asString();

    // the taskType
    if (!root.isMember("type"))
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment("Must specify the taskType"));
    }
    if (root.isMember("enableOutputExists"))
    {
        m_enableOutputExists = root["enableOutputExists"].asBool();
    }
    checkNull(root["type"], "taskType");
    m_type = root["type"].asUInt();
    // the taskAlgorithm
    if (!root.isMember("algorithm"))
    {
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment("Must specify the taskAlgorithm"));
    }
    checkNull(root["algorithm"], "taskAlgorithm");
    m_algorithm = root["algorithm"].asUInt();

    // the parameters
    if (root.isMember("params") && !root["params"].isNull())
    {
        m_param = root["params"].asString();
    }
    // sync result to peer or not
    if (root.isMember("syncResult") && !root["syncResult"].isNull())
    {
        m_syncResultToPeer = root["syncResult"].asBool();
    }
    if (root.isMember("lowBandwidth") && !root["lowBandwidth"].isNull())
    {
        m_lowBandwidth = root["lowBandwidth"].asBool();
    }
    if (root.isMember("receiverList") && !root["receiverList"].isNull() &&
        root["receiverList"].isArray())
    {
        Json::Value _receivers = root["receiverList"];
        for (const auto& r : _receivers)
        {
            if (r.isString())
            {
                m_receiverLists.push_back(r.asString());
            }
        }
    }

    checkNull(root["parties"], "parties");
    // the peer-parties-info
    if (!root.isMember("parties"))
    {
        return;
    }
    auto const& peersPartyEntry = root["parties"];
    for (Json::ArrayIndex i = 0; i < peersPartyEntry.size(); i++)
    {
        auto partyInfo = std::make_shared<PartyResource>();
        decodePartyInfo(partyInfo, peersPartyEntry[i]);
        // obtain the self-party
        if (partyInfo->id() == m_selfPartyID)
        {
            m_self = partyInfo;
        }
        else
        {
            m_peerParties[partyInfo->id()] = partyInfo;
        }
    }
    if (!m_self)
    {
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment("Must specify the self-party-info"));
    }
}

// decode the party-info
void JsonTaskImpl::decodePartyInfo(PartyResource::Ptr _partyInfo, Json::Value const& _root)
{
    // the partyIndex
    if (_root.isMember("partyIndex") && !_root["partyIndex"].isNull())
    {
        _partyInfo->setPartyIndex(_root["partyIndex"].asUInt());
    }
    // the partyID
    if (!_root.isMember("id"))
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment("Must specify the partyID"));
    }
    checkNull(_root["id"], "partyID");
    _partyInfo->setId(_root["id"].asString());
    // the desc
    if (_root.isMember("desc"))
    {
        _partyInfo->setDesc(_root["desc"].asString());
    }
    // the data-resource
    if (_root.isMember("data") && !_root["data"].isNull())
    {
        _partyInfo->setDataResource(decodeDataResource(_root["data"]));
    }
}

// decode the data-resource-info
DataResource::Ptr JsonTaskImpl::decodeDataResource(Json::Value const& _root)
{
    auto dataResource = std::make_shared<DataResource>();
    // the resourceID
    if (!_root.isMember("id"))
    {
        BOOST_THROW_EXCEPTION(
            InvalidParameter() << errinfo_comment("Must specify the id for the data-resource"));
    }
    checkNull(_root["id"], "data-resource-id");
    dataResource->setResourceID(_root["id"].asString());
    // the resource-desc
    if (_root.isMember("input") && !_root["input"].isNull())
    {
        // decode the input
        auto const& descEntry = _root["input"];
        auto dataDesc = std::make_shared<DataResourceDesc>();
        decodeDataResourceDesc(dataDesc, descEntry);
        dataResource->setDesc(dataDesc);
    }
    // decode the output
    if (_root.isMember("output") && !_root["output"].isNull())
    {
        auto outputDesc = std::make_shared<DataResourceDesc>();
        decodeDataResourceDesc(outputDesc, _root["output"]);
        dataResource->setOutputDesc(outputDesc);
    }
    // decode the rawData
    if (_root.isMember("rawData") && !_root["rawData"].isNull())
    {
        DataResource::OriginData originData;
        for (auto& members : _root["rawData"])
        {
            std::vector<std::string> data;
            for (auto& member : members)
            {
                data.emplace_back(member.asString());
            }
            originData.emplace_back(data);
        }
        dataResource->setRawData(originData);
    }
    return dataResource;
}

void JsonTaskImpl::decodeDataResourceDesc(DataResourceDesc::Ptr _desc, Json::Value const& _value)
{
    // the dataResourceType
    // Note: the data-resource is not required for the psi-server
    if (_value.isMember("type"))
    {
        _desc->setType(_value["type"].asUInt());
    }
    if (_value.isMember("path"))
    {
        auto path = _value["path"].asString();
        if (_desc->type() == (int)DataResourceType::FILE)
        {
            if (path.find("..") != std::string::npos)
            {
                BOOST_THROW_EXCEPTION(
                    InvalidParameter() << errinfo_comment("The \"..\" cannot be in the path"));
            }
            if (path.rfind("/", 0) == 0)
            {
                _desc->setPath(path);
            }
            else
            {
                boost::filesystem::path prePath(m_prePath);
                boost::filesystem::path inputPath(path);
                boost::filesystem::path filePath(prePath / inputPath);
                _desc->setPath(filePath.string());
            }
        }
        else
        {
            _desc->setPath(_value["path"].asString());
        }
    }
    if (_value.isMember("command"))
    {
        _desc->setAccessCommand(_value["command"].asString());
    }
    if (_value.isMember("fileID"))
    {
        _desc->setFileID(_value["fileID"].asString());
    }
    if (_value.isMember("fileMd5"))
    {
        _desc->setFileMd5(_value["fileMd5"].asString());
    }
    if (_value.isMember("bizSeqNo"))
    {
        _desc->setBizSeqNo(_value["bizSeqNo"].asString());
    }

    // decode the connection option
    if (_desc->type() == (int)DataResourceType::MySQL)
    {
        if (!_value.isMember("source"))
        {
            return;
            //            BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
            //                                      "The MySQL type resource must define the
            //                                      source"));
        }
        auto const& connectionOption = _value["source"];
        SQLConnectionOption option;
        option.host = connectionOption["host"].asString();
        checkNonEmptyField("host", option.host);

        option.port = connectionOption["port"].asUInt();
        option.user = connectionOption["user"].asString();
        checkNonEmptyField("user", option.user);

        option.password = connectionOption["password"].asString();
        checkNonEmptyField("password", option.password);

        option.database = connectionOption["database"].asString();
        checkNonEmptyField("database", option.database);

        _desc->setSQLConnectionOption(std::make_shared<SQLConnectionOption>(option));
    }
    // decode the hdfs
    if (_desc->type() == (int)DataResourceType::HDFS)
    {
        if (!_value.isMember("source"))
        {
            return;
            //            BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment(
            //                                      "The HDFS type resource must define the
            //                                      source"));
        }
        auto option = std::make_shared<FileStorageConnectionOption>();
        auto const& fileStorageOpt = _value["source"];
        option->nameNode = fileStorageOpt["nameNode"].asString();
        checkNonEmptyField("nameNode", option->nameNode);
        option->nameNodePort = fileStorageOpt["nameNodePort"].asUInt();
        option->userName = fileStorageOpt["user"].asString();
        checkNonEmptyField("user", option->userName);

        option->token = fileStorageOpt["token"].asString();
        _desc->setFileStorageConnectOption(option);
    }
}

std::string JsonTaskImpl::encode() const
{
    Json::Value taskInfo;
    // the taskID
    taskInfo["taskID"] = m_id;
    // the taskType
    taskInfo["type"] = m_type;
    // the task-algorithm
    taskInfo["algorithm"] = m_algorithm;
    // params
    taskInfo["params"] = m_param;
    // sync-result or not
    taskInfo["syncResult"] = m_syncResultToPeer;
    taskInfo["lowBandwidth"] = m_lowBandwidth;
    taskInfo["enableOutputExists"] = m_enableOutputExists;

    Json::Value receiverList;
    for (auto const& it : m_receiverLists)
    {
        receiverList.append(it);
    }
    taskInfo["receiverList"] = receiverList;

    Json::Value partiesInfo(Json::arrayValue);
    // encode the partyInfo
    if (m_self)
    {
        Json::Value selfPartyInfo;
        encodePartyInfo(selfPartyInfo, m_self);
        partiesInfo.append(selfPartyInfo);
    }
    for (auto const& it : m_peerParties)
    {
        Json::Value partyInfo;
        encodePartyInfo(partyInfo, it.second);
        partiesInfo.append(partyInfo);
    }
    taskInfo["parties"] = partiesInfo;
    Json::FastWriter fastWriter;
    return fastWriter.write(taskInfo);
}

void JsonTaskImpl::encodePartyInfo(Json::Value& _value, PartyResource::Ptr _party) const
{
    // the partyIndex
    _value["partyIndex"] = _party->partyIndex();
    // the partyID
    _value["id"] = _party->id();
    // the desc
    _value["desc"] = _party->desc();
    // the dataResource
    encodeDataResource(_value["data"], _party->dataResource());
}

void JsonTaskImpl::encodeDataResource(
    Json::Value& _value, DataResource::ConstPtr _dataResource) const
{
    // the dataID
    _value["id"] = _dataResource->resourceID();
    // the desc
    if (_dataResource->desc())
    {
        // the type
        _value["input"] = encodeDataResourceDesc(_dataResource->desc());
    }
    if (_dataResource->outputDesc())
    {
        _value["output"] = encodeDataResourceDesc(_dataResource->outputDesc());
    }
    if (!_dataResource->rawData().empty())
    {
        Json::Value jsonOriginData;
        for (auto& dataVector : _dataResource->rawData())
        {
            Json::Value jsonData;
            for (auto& data : dataVector)
            {
                jsonData.append(data);
            }
            jsonOriginData.append(jsonData);
        }
        _value["rawData"] = jsonOriginData;
    }
}

Json::Value JsonTaskImpl::encodeDataResourceDesc(DataResourceDesc::ConstPtr _desc) const
{
    Json::Value result;
    result["type"] = _desc->type();
    result["path"] = _desc->path();
    result["command"] = _desc->accessCommand();
    // encode the sql connection option
    Json::Value connectOption;
    if (_desc->sqlConnectionOption())
    {
        connectOption["host"] = _desc->sqlConnectionOption()->host;
        connectOption["port"] = _desc->sqlConnectionOption()->port;
        connectOption["user"] = _desc->sqlConnectionOption()->user;
        connectOption["password"] = _desc->sqlConnectionOption()->password;
        connectOption["database"] = _desc->sqlConnectionOption()->database;
    }
    // encode the file-storage-connection-option
    auto const& fileStorageOpt = _desc->fileStorageConnectionOption();
    if (fileStorageOpt)
    {
        connectOption["nameNode"] = fileStorageOpt->nameNode;
        connectOption["nameNodePort"] = fileStorageOpt->nameNodePort;
        connectOption["user"] = fileStorageOpt->userName;
        connectOption["token"] = fileStorageOpt->token;
    }
    result["source"] = connectOption;
    return result;
}

PartyResource::ConstPtr JsonTaskImpl::getPartyByID(std::string const& _partyID) const
{
    if (m_self && m_self->id() == _partyID)
    {
        return m_self;
    }
    if (m_peerParties.count(_partyID))
    {
        return m_peerParties.at(_partyID);
    }
    return nullptr;
}

PartyResource::ConstPtr JsonTaskImpl::getPartyByIndex(uint16_t _partyIndex) const
{
    if (m_self->partyIndex() == _partyIndex)
    {
        return m_self;
    }
    for (const auto& peerParty : m_peerParties)
    {
        if (peerParty.second->partyIndex() == _partyIndex)
        {
            return peerParty.second;
        }
    }
    return nullptr;
}
