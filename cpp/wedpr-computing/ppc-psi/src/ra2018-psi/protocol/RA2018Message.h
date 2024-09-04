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
 * @file RA2018Message.h
 * @author: yujiechen
 * @date 2022-11-9
 */
#pragma once
#include "../../psi-framework/protocol/PSIMessage.h"
#include "../Common.h"
#include "../core/CuckooFilterInfo.h"
#include "wedpr-protocol/tars/Common.h"

namespace ppc::psi
{
class RA2018FilterMessage : public PSIMessage
{
public:
    using Ptr = std::shared_ptr<RA2018FilterMessage>;
    RA2018FilterMessage() : PSIMessage() {}
    explicit RA2018FilterMessage(uint32_t _type) : PSIMessage(_type) {}
    explicit RA2018FilterMessage(std::function<ppctars::PSIMessage*()> _inner)
      : PSIMessage(std::move(_inner))
    {
        decodeCuckooFilterInfos();
    }

    explicit RA2018FilterMessage(bcos::bytesConstRef _data) : RA2018FilterMessage()
    {
        decode(_data);
    }

    virtual void setFilterInfo(std::vector<CuckooFilterInfo::Ptr>&& _filterInfos)
    {
        m_cuckooFilterInfos = std::move(_filterInfos);
    }

    virtual void appendFilterInfo(CuckooFilterInfo::Ptr _filterInfo)
    {
        m_cuckooFilterInfos.emplace_back(std::move(_filterInfo));
    }

    virtual void setCuckooFilterSize(int32_t _cuckooFilterSize)
    {
        m_inner()->cuckooFilterSize = _cuckooFilterSize;
    }
    virtual int32_t cuckooFilterSize() const { return m_inner()->cuckooFilterSize; }

    virtual std::vector<CuckooFilterInfo::Ptr>& mutableFilterInfo() { return m_cuckooFilterInfos; }
    virtual std::vector<CuckooFilterInfo::Ptr> const& filterInfo() { return m_cuckooFilterInfos; }

    bcos::bytesPointer encode() const override
    {
        m_inner()->cuckooFilterInfo.clear();
        // encode m_cuckooFilterInfos
        for (auto& info : m_cuckooFilterInfos)
        {
            ppctars::TarsCuckooFilter filter;
            filter.id = info->filterID();
            filter.hash = std::vector<tars::Char>(info->hash().begin(), info->hash().end());
            if (info->cuckooFilterData().empty() && info->cuckooFilter())
            {
                auto encodedData = info->cuckooFilter()->serialize();
                std::move(encodedData.begin(), encodedData.end(), std::back_inserter(filter.data));
            }
            else if (!info->cuckooFilterData().empty())
            {
                auto data = info->cuckooFilterData();
                std::move(data.begin(), data.end(), std::back_inserter(filter.data));
            }
            m_inner()->cuckooFilterInfo.emplace_back(std::move(filter));
        }
        return PSIMessage::encode();
    }

    // decode the RA2018FilterMessage
    void decode(bcos::bytesConstRef _data) override
    {
        PSIMessage::decode(_data);
        // decode m_cuckooFilterInfos
        decodeCuckooFilterInfos();
    }

protected:
    void decodeCuckooFilterInfos()
    {
        m_cuckooFilterInfos.clear();
        for (auto& filterInfo : m_inner()->cuckooFilterInfo)
        {
            DefaultCukooFilterPtr filter = nullptr;
            // decode the cuckoo-filter
            if (!filterInfo.data.empty())
            {
                filter = std::make_shared<DefaultCukooFilter>(bcos::bytesConstRef(
                    (const bcos::byte*)filterInfo.data.data(), filterInfo.data.size()));
            }
            bcos::bytes hashData(filterInfo.hash.begin(), filterInfo.hash.end());
            auto cuckooFilterInfo =
                std::make_shared<CuckooFilterInfo>(filterInfo.id, hashData, filter);
            m_cuckooFilterInfos.emplace_back(std::move(cuckooFilterInfo));
        }
    }

private:
    mutable std::vector<CuckooFilterInfo::Ptr> m_cuckooFilterInfos;
};

// the factory implementation
class RA2018MessageFactory : public PSIMessageFactoryImpl
{
public:
    using Ptr = std::shared_ptr<RA2018MessageFactory>;
    RA2018MessageFactory() : PSIMessageFactoryImpl() {}
    ~RA2018MessageFactory() override = default;

    virtual RA2018FilterMessage::Ptr createRA2018FilterMessage(uint32_t _packetType)
    {
        return std::make_shared<RA2018FilterMessage>(_packetType);
    }

    PSIMessageInterface::Ptr decodePSIMessage(bcos::bytesConstRef _data) override
    {
        auto inner = [inner = ppctars::PSIMessage()]() mutable { return &inner; };
        tars::TarsInputStream<tars::BufferReader> input;
        input.setBuffer((const char*)_data.data(), _data.size());
        inner()->readFrom(input);
        switch (inner()->packetType)
        {
        case (uint32_t)RA2018PacketType::CuckooFilterRequest:
        case (uint32_t)RA2018PacketType::CuckooFilterResponse:
        case (uint32_t)RA2018PacketType::MissingCuckooFilterRequest:
        case (uint32_t)RA2018PacketType::MissingCuckooFilterResponse:
            return std::make_shared<RA2018FilterMessage>(inner);
        case (uint32_t)RA2018PacketType::EvaluateRequest:
        case (uint32_t)RA2018PacketType::EvaluateResponse:
            return std::make_shared<PSIMessage>(inner);
        default:
            return decodePSIBaseMessage(inner);
        }
    }
};
}  // namespace ppc::psi