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
 * @file SQLStorage.h
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include "../io/DataBatch.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/Error.h>
#include <memory>
namespace ppc::storage
{
enum FieldDataType : int
{
    SINT = 0,
    UINT = 1,
    DOUBLE = 2,
    FLOAT = 3,
    BYTES = 4,
    STRING = 5,
    TERMINATE,
};

using MetaFieldType = std::vector<std::string>;
class QueryResult
{
public:
    using Ptr = std::shared_ptr<QueryResult>;
    QueryResult() = default;
    virtual ~QueryResult() = default;

    MetaFieldType const& metaData() const { return m_metaData; }
    std::vector<ppc::io::DataBatch::Ptr> const& data() const { return m_data; }

    MetaFieldType& mutableMetaData() { return m_metaData; }
    std::vector<ppc::io::DataBatch::Ptr>& mutableData() { return m_data; }

    uint64_t size() const { return m_data.size(); }

private:
    // the meta-data
    MetaFieldType m_metaData;
    // the data
    std::vector<ppc::io::DataBatch::Ptr> m_data;
};

class Statement
{
public:
    using Ptr = std::shared_ptr<Statement>;
    Statement() = default;
    virtual ~Statement() = default;
    // get the statementContext
    virtual void* statementContext() = 0;
    virtual void* takeSession() = 0;
};

class SQLResultContext
{
public:
    using Ptr = std::shared_ptr<SQLResultContext>;
    SQLResultContext() = default;
    virtual ~SQLResultContext() = default;

    // get the sql-result
    virtual void* result() = 0;
};

// Only the synchronous interface is provided here
class SQLStorage
{
public:
    using Ptr = std::shared_ptr<SQLStorage>;

    SQLStorage() = default;
    virtual ~SQLStorage() = default;

    // use the database
    virtual void useDataBase(const char* _database) = 0;

    // exec the sql-query-command
    virtual QueryResult::Ptr execQuery(bool _parseByColumn, const char* _command, ...) const = 0;
    // exec the sql-write-command: insert/update/delete
    virtual void execCommit(const char* _command, ...) const = 0;

    virtual Statement::Ptr generateStatement(const char* _command, ...) const = 0;
    virtual SQLResultContext::Ptr execStatement(Statement::Ptr _statement) const = 0;
    virtual void appendStatement(Statement::Ptr _statement, ...) const = 0;
    virtual std::string const& databaseName() const = 0;
};

class SQLStorageFactory
{
public:
    using Ptr = std::shared_ptr<SQLStorageFactory>;
    SQLStorageFactory() = default;
    virtual ~SQLStorageFactory() = default;

    virtual SQLStorage::Ptr createSQLStorage(ppc::protocol::DataResourceType _type,
        ppc::protocol::SQLConnectionOption::Ptr const& _option) = 0;
};
}  // namespace ppc::storage