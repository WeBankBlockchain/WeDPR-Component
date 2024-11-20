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
 * @file DataBatch.h
 * @author: yujiechen
 * @date 2022-10-17
 */
#pragma once
#include "../Common.h"
#include <bcos-utilities/Common.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <memory>
#include <vector>

namespace ppc::io
{
using DataType = std::variant<std::string, bcos::bytes, uint64_t, int64_t, double, float>;

enum DataSchema
{
    String,
    Bytes,
    Sint,
    Uint,
    Double,
    Float,
    Null,  // the data-schema not setted
};

// the DataBatch
class DataBatch
{
public:
    using Ptr = std::shared_ptr<DataBatch>;
    using ConstPtr = std::shared_ptr<const DataBatch>;
    DataBatch() : m_data(std::make_shared<std::vector<DataType>>()) {}
    DataBatch(std::vector<DataType> _data) : DataBatch() { *m_data = std::move(_data); }
    ~DataBatch() = default;

    template <typename T>
    void setData(std::vector<T>&& _data)
    {
        m_data->clear();
        for (auto& it : _data)
        {
            append(std::move(it));
        }
    }

    template <typename T>
    void setData(std::vector<T> const& _data)
    {
        m_data->clear();
        for (auto const& it : _data)
        {
            append(it);
        }
    }

    // append to different line
    template <typename T>
    void append(T _data)
    {
        if constexpr (std::is_same<T, std::string>::value || std::is_same<T, bcos::bytes>::value)
        {
            m_capacityBytes += _data.size();
        }
        else
        {
            m_capacityBytes += sizeof(_data);
        }
        m_data->emplace_back(_data);
    }

    // append to the last line
    // Note: only bcos::bytes, std::string can use this function
    template <typename T>
    void appendToLine(T&& _data)
    {
        auto dataIndex = m_data->size() - 1;
        auto& data = std::get<T>((*m_data)[dataIndex]);
        data.insert(data.end(), _data.begin(), _data.end());
        if constexpr (std::is_same<T, std::string>::value || std::is_same<T, bcos::bytes>::value)
        {
            m_capacityBytes += _data.size();
        }
    }

    // Note: can't pass-in a invalid index
    template <typename T>
    T const& get(uint64_t _index) const
    {
        return std::get<T>(m_data->at(_index));
    }

    // Note: This introduces additional copy overhead
    // convert to bytes
    bcos::bytes getBytes(uint64_t _index)
    {
        if (m_dataSchema == DataSchema::Null)
        {
            BOOST_THROW_EXCEPTION(DataSchemaNotSetted() << bcos::errinfo_comment(
                                      "Must set DataSchema before calling getBytes!"));
        }
        switch (m_dataSchema)
        {
        case DataSchema::String:
        {
            auto const& item = get<std::string>(_index);
            return bcos::bytes(item.begin(), item.end());
        }
        case DataSchema::Bytes:
        {
            return get<bcos::bytes>(_index);
        }
        case DataSchema::Uint:
        {
            auto const& value = get<uint64_t>(_index);
            auto result = boost::lexical_cast<std::string>(value);
            return bcos::bytes(result.begin(), result.end());
        }
        case DataSchema::Sint:
        {
            auto const& value = get<int64_t>(_index);
            auto result = boost::lexical_cast<std::string>(value);
            return bcos::bytes(result.begin(), result.end());
        }
        case DataSchema::Double:
        {
            auto const& value = get<double>(_index);
            auto result = boost::lexical_cast<std::string>(value);
            return bcos::bytes(result.begin(), result.end());
        }
        case DataSchema::Float:
        {
            auto const& value = get<float>(_index);
            auto result = boost::lexical_cast<std::string>(value);
            return bcos::bytes(result.begin(), result.end());
        }
        default:
        {
            BOOST_THROW_EXCEPTION(
                UnsupportedDataSchema() << bcos::errinfo_comment("unsupported data schema"));
        }
        }
    }

    uint64_t size() const { return m_data->size(); }

    template <typename T>
    uint64_t elementSize(uint64_t _offset) const
    {
        if constexpr (std::is_same<T, std::string>::value || std::is_same<T, bcos::bytes>::value)
        {
            return std::get<T>((*m_data)[_offset]).size();
        }
        else
        {
            return sizeof(T);
        }
    }
    uint64_t capacityBytes() const { return m_capacityBytes; }

    void resize(uint64_t _size) { m_data->resize(_size); }

    template <typename T>
    void resizeElement(uint64_t _index, uint64_t _size)
    {
        std::get<T>((*m_data)[_index]).resize(_size);
    }

    template <typename T>
    void set(uint64_t _offset, T&& _data)
    {
        if constexpr (std::is_same<T, std::string>::value || std::is_same<T, bcos::bytes>::value)
        {
            auto orgDataSize = elementSize<T>(_offset);
            m_capacityBytes -= orgDataSize;
            m_capacityBytes += _data.size();
        }
        (*m_data)[_offset].emplace<T>(std::move(_data));
    }

    void setDataSchema(DataSchema _dataSchema) { m_dataSchema = _dataSchema; }

    DataSchema getDataSchema() const { return m_dataSchema; }


    std::shared_ptr<std::vector<DataType>>& mutableData() { return m_data; }

    void release()
    {
        if (!m_data)
        {
            return;
        }
        m_data->clear();
        m_data.reset();
    }

private:
    std::shared_ptr<std::vector<DataType>> m_data;
    uint64_t m_capacityBytes = 0;
    DataSchema m_dataSchema = DataSchema::Null;
};
}  // namespace ppc::io