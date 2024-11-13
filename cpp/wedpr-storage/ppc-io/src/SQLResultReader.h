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
 * @file SQLResultReader.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once
#include "ppc-framework/io/LineReader.h"
#include "ppc-framework/storage/SQLStorage.h"
#include <boost/core/ignore_unused.hpp>

namespace ppc::io
{
class SQLResultReader : public LineReader
{
public:
    using Ptr = std::shared_ptr<SQLResultReader>;
    SQLResultReader(ppc::storage::QueryResult::Ptr _result, bool _parseByColumn)
      : m_result(std::move(_result)), m_parseByColumn(_parseByColumn)
    {}
    ~SQLResultReader() override {}
    // get the next _offset line data
    // Note: the _schema is not used by mysql-result-reader
    DataBatch::Ptr next(int64_t _offset = 0, DataSchema _schema = DataSchema::Bytes) override
    {
        boost::ignore_unused(_schema);
        if ((int64_t)m_result->data().size() <= _offset)
        {
            return nullptr;
        }
        return m_result->mutableData().at(_offset);
    }

    // the capacity the query-result
    uint64_t capacity() const override { return m_result->data().size(); }
    uint64_t columnSize() const override
    {
        if (m_parseByColumn)
        {
            return m_result->data().size();
        }
        return 1;
    }

    ppc::protocol::DataResourceType type() const override
    {
        return ppc::protocol::DataResourceType::MySQL;
    }

    bcos::bytes readBytes() override
    {
        throw std::runtime_error("SQLResultReader: unimplemented readBytes!");
    }
    bool readFinished() const override { return true; }

private:
    ppc::storage::QueryResult::Ptr m_result;
    bool m_parseByColumn;
};
}  // namespace ppc::io