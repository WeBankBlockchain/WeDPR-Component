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
 * @file MySQLStorage.cpp
 * @author: yujiechen
 * @date 2022-10-25
 */
#include "MySQLStorage.h"
#include <stdarg.h>
#include <sstream>

using namespace ppc::storage;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace bcos;

MySQLStorage::MySQLStorage(
    ppc::protocol::SQLConnectionOption::Ptr const& _opt, uint16_t _sessionPoolSize)
  : m_opt(_opt),
    m_sessionPoolSize(_sessionPoolSize),
    m_timer(std::make_shared<bcos::Timer>(1000 * 60 * 1, "mysqlTimer"))
{
    if (!m_opt)
    {
        BOOST_THROW_EXCEPTION(NotSetMySQLConnectionOption()
                              << errinfo_comment("Must set the mysql-connection-option!"));
    }
    if (m_opt->database.empty())
    {
        useDataBase(nullptr);
    }
    else
    {
        useDataBase(m_opt->database.c_str());
    }

    if (m_timer)
    {
        m_timer->registerTimeoutHandler([this] { refreshConnection(); });
        m_timer->start();
    }
}

void MySQLStorage::refreshConnection()
{
    MYSQL_STORAGE_LOG(INFO) << LOG_DESC("refreshConnection");

    if (m_opt->database.empty())
    {
        useDataBase(nullptr);
    }
    else
    {
        useDataBase(m_opt->database.c_str());
    }

    if (m_timer)
    {
        m_timer->restart();
    }
}

// use the database
void MySQLStorage::useDataBase(const char* _database)
{
    try
    {
        m_opt->database = _database ? _database : "";
        // destory the current session
        destroyAllSession();
        insertSession(createSession());
        MYSQL_STORAGE_LOG(INFO) << LOG_DESC("use mysql success")
                                << LOG_KV("connectOpt", m_opt->desc());
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(ConnectMySQLError() << errinfo_comment(
                                  "connect to mysql error: " + boost::diagnostic_information(e)));
    }
}

mysqlx_session_t* MySQLStorage::createSession() const
{
    const char* dataBase = m_opt->database.empty() ? nullptr : m_opt->database.c_str();
    try
    {
        return connectDataBase(dataBase, false);
    }
    catch (std::exception const& e)
    {
        return connectDataBase(dataBase, true);
    }
}

mysqlx_session_t* MySQLStorage::connectDataBase(const char* _database, bool _disableSSl) const
{
    // allocate new session according to _database
    mysqlx_error_t* error = NULL;
    std::stringstream conn;
    conn << m_opt->user << ":" << m_opt->password << "@" << m_opt->host;
    if (_database)
    {
        conn << "/" << _database;
    }
    if (_disableSSl)
    {
        conn << "?ssl-mode=disabled&mysql41";
    }
    MYSQL_STORAGE_LOG(INFO) << LOG_DESC("MySQLStorage connect") << LOG_KV("conn", conn.str());
    auto session = mysqlx_get_session_from_url(conn.str().c_str(), &error);
    if (!session)
    {
        auto errorMsg = std::string(mysqlx_error_message(error));
        MYSQL_STORAGE_LOG(ERROR) << LOG_DESC("connect to mysql error") << m_opt->desc()
                                 << LOG_KV("code", mysqlx_error_num(error))
                                 << LOG_KV("msg", errorMsg);
        // free the error
        mysqlx_free(error);
        BOOST_THROW_EXCEPTION(
            ConnectMySQLError() << errinfo_comment("connect to mysql error: " + errorMsg));
    }
    return session;
}

// exec the sql-query-command
QueryResult::Ptr MySQLStorage::execQuery(bool _parseByColumn, const char* _command, ...) const
{
    // parse the args
    va_list args;
    va_start(args, _command);
    auto result = execSQL(_command, args);
    va_end(args);
    // query no data
    uint32_t columnSize = mysqlx_column_get_count((mysqlx_result_t*)result->result());
    if (!columnSize)
    {
        return nullptr;
    }
    MYSQL_STORAGE_LOG(TRACE) << LOG_DESC("execQuery success: ") << LOG_KV("command", _command)
                             << LOG_KV("columnSize", columnSize);
    auto queryResult = std::make_shared<QueryResult>();
    parseMetaData(queryResult, result, columnSize);
    if (_parseByColumn)
    {
        parseResultByColumn(queryResult, result, columnSize);
    }
    parseResultByRow(queryResult, result, columnSize);
    return queryResult;
}

void MySQLStorage::parseMetaData(
    QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result, uint32_t _columnSize) const
{
    for (uint32_t col = 0; col < _columnSize; col++)
    {
        _queryResult->mutableMetaData().emplace_back(
            std::string(mysqlx_column_get_name((mysqlx_result_t*)_result->result(), col)));
    }
}

// exec the sql-write-command: insert/update/delete
void MySQLStorage::execCommit(const char* _command, ...) const
{
    // parse the args
    va_list args;
    va_start(args, _command);
    auto result = execSQL(_command, args);
    va_end(args);
    MYSQL_STORAGE_LOG(TRACE) << LOG_DESC("execCommit success: ") << _command;
}

Statement::Ptr MySQLStorage::generateStatement(const char* _command, ...) const
{
    // parse the args
    va_list args;
    va_start(args, _command);
    auto result = generateSQLStatement(_command, args);
    va_end(args);
    return result;
}

SQLResultContext::Ptr MySQLStorage::execStatement(Statement::Ptr _statement) const
{
    auto stmt = (mysqlx_stmt_t*)(_statement->statementContext());
    // execute the sql command
    auto res = mysqlx_execute(stmt);
    if (!res)
    {
        auto errorMsg = mysqlx_error_message((void*)stmt);
        MYSQL_STORAGE_LOG(ERROR) << LOG_DESC("execStatement error")
                                 << LOG_KV("code", mysqlx_error((void*)stmt))
                                 << LOG_KV("msg", errorMsg ? errorMsg : "None");
        BOOST_THROW_EXCEPTION(ExecMySQLError() << bcos::errinfo_comment(
                                  "exec sql error: " + std::string(errorMsg ? errorMsg : "None")));
    }
    MYSQL_STORAGE_LOG(TRACE) << LOG_DESC("execStatement success");

    auto result = std::make_shared<MySQLResultContext>((mysqlx_result_t*)res);
    // takeSession from the statement
    result->setSession(_statement->takeSession());
    result->setReturnSessionCallback(
        boost::bind(&MySQLStorage::returnSession, this, boost::placeholders::_1));
    return result;
}

void MySQLStorage::appendStatement(Statement::Ptr _statement, ...) const
{
    va_list args;
    va_start(args, _statement);
    appendStatement(_statement, args);
    va_end(args);
}

void MySQLStorage::appendStatement(Statement::Ptr _stmt, va_list& args) const
{
    auto stmt = (mysqlx_stmt_t*)(_stmt->statementContext());
    // get the data type
    int type = va_arg(args, int);
    while (type != (int)FieldDataType::TERMINATE)
    {
        // get the value
        switch (type)
        {
        case FieldDataType::SINT:
        {
            bindStmt(stmt, PARAM_SINT(va_arg(args, int64_t)), PARAM_END);
            break;
        }
        case FieldDataType::UINT:
        {
            bindStmt(stmt, PARAM_UINT(va_arg(args, uint64_t)), PARAM_END);
            break;
        }
        case FieldDataType::DOUBLE:
        case FieldDataType::FLOAT:
        {
            bindStmt(stmt, PARAM_FLOAT(va_arg(args, double)), PARAM_END);
            break;
        }
        case FieldDataType::STRING:
        {
            bindStmt(stmt, PARAM_STRING(va_arg(args, char*)), PARAM_END);
            break;
        }
        case FieldDataType::BYTES:
        {
            bcos::byte* data = va_arg(args, bcos::byte*);
            size_t len = va_arg(args, size_t);
            bindStmt(stmt, PARAM_BYTES(data, len), PARAM_END);
            break;
        }
        default:
        {
            BOOST_THROW_EXCEPTION(
                ExecMySQLError() << bcos::errinfo_comment(
                    "unsupported data type for statement: " + std::to_string(type)));
        }
        }
        type = va_arg(args, int);
    }
}

Statement::Ptr MySQLStorage::generateSQLStatement(const char* _command, va_list& args) const
{
    auto session = allocateSession();
    // Note: the session will be return back when statement destroied
    auto statement = std::make_shared<MySQLStatement>(
        mysqlx_sql_new(session, _command, MYSQLX_NULL_TERMINATED), session);
    // for return session
    statement->setReturnSessionCallback(
        boost::bind(&MySQLStorage::returnSession, this, boost::placeholders::_1));
    MYSQL_STORAGE_LOG(TRACE) << LOG_DESC("generateSQLStatement") << LOG_KV("command", _command);
    if (!statement->statementContext())
    {
        auto errorMsgPtr = mysqlx_error_message(mysqlx_error((void*)session));
        std::string errorMsg = std::string(errorMsgPtr ? errorMsgPtr : "None");
        MYSQL_STORAGE_LOG(ERROR) << LOG_DESC("generateSQLStatement error")
                                 << LOG_KV("command", _command)
                                 << LOG_KV("code", mysqlx_error((void*)session))
                                 << LOG_KV("msg", errorMsg);
        BOOST_THROW_EXCEPTION(
            ExecMySQLError() << bcos::errinfo_comment("generate sql statement error: " + errorMsg));
    }
    appendStatement(statement, args);
    return statement;
}

void MySQLStorage::parseResult(ppc::io::DataBatch::Ptr _result,
    SQLResultContext::Ptr const& _mysqlResult, mysqlx_row_t* _rowData, uint32_t _col) const
{
    auto colType = mysqlx_column_get_type((mysqlx_result_t*)_mysqlResult->result(), _col);
    switch (colType)
    {
    case mysqlx_data_type_enum::MYSQLX_TYPE_BOOL:
    case mysqlx_data_type_enum::MYSQLX_TYPE_SINT:
    {
        int64_t element;
        auto ret = mysqlx_get_sint(_rowData, _col, &element);
        appendResult(_rowData, _result, ret, std::move(element));
        _result->setDataSchema(DataSchema::Sint);
        break;
    }
    case mysqlx_data_type_enum::MYSQLX_TYPE_UINT:
    {
        uint64_t element;
        auto ret = mysqlx_get_uint(_rowData, _col, &element);
        appendResult(_rowData, _result, ret, std::move(element));
        _result->setDataSchema(DataSchema::Uint);
        break;
    }
    case mysqlx_data_type_enum::MYSQLX_TYPE_DOUBLE:
    {
        double element;
        auto ret = mysqlx_get_double(_rowData, _col, &element);
        appendResult(_rowData, _result, ret, std::move(element));
        _result->setDataSchema(DataSchema::Double);
        break;
    }
    case mysqlx_data_type_enum::MYSQLX_TYPE_FLOAT:
    {
        float element;
        auto ret = mysqlx_get_float(_rowData, _col, &element);
        appendResult(_rowData, _result, ret, std::move(element));
        _result->setDataSchema(DataSchema::Float);
        break;
    }
    case mysqlx_data_type_enum::MYSQLX_TYPE_BYTES:
    case mysqlx_data_type_enum::MYSQLX_TYPE_STRING:
    {
        bool readFinished = false;
        uint64_t offset = 0;
        _result->append(bcos::bytes());
        // Note: the huge data larger then c_maxElementLen will not been readed-finished at-one-time
        while (!readFinished)
        {
            bcos::bytes element;
            element.resize(c_maxElementLen);
            // Note: the elementLen should not be zero, otherwise, will trigger "The output buffer
            // cannot have zero length" error
            size_t elementLen = element.size();
            auto ret = mysqlx_get_bytes(_rowData, _col, offset, (void*)element.data(), &elementLen);
            offset += elementLen;
            readFinished = appendResult(_rowData, _result, ret, bcos::bytes(), false);
            // append the element to line
            _result->appendToLine(std::move(element));
        }
        // Note: the last one 0x00-bytes means terminate, should been ignored
        _result->resizeElement<bcos::bytes>(_result->size() - 1, offset - 1);
        _result->setDataSchema(DataSchema::Bytes);
        break;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(ParseMySQLResultError() << bcos::errinfo_comment(
                                  "unsupported data type: " + std::to_string(colType)));
    }
    }
}

void MySQLStorage::parseResultByColumn(
    QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result, uint32_t _columnSize) const
{
    _queryResult->mutableData().resize(_columnSize);
    for (uint64_t i = 0; i < _columnSize; i++)
    {
        auto batchData = std::make_shared<ppc::io::DataBatch>();
        _queryResult->mutableData()[i] = (std::move(batchData));
    }

    while (auto rowData = mysqlx_row_fetch_one((mysqlx_result_t*)_result->result()))
    {
        for (uint32_t col = 0; col < _columnSize; col++)
        {
            parseResult((_queryResult->mutableData())[col], _result, rowData, col);
        }
    }
}

void MySQLStorage::parseResultByRow(
    QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result, uint32_t _columnSize) const
{
    while (auto rowData = mysqlx_row_fetch_one((mysqlx_result_t*)_result->result()))
    {
        auto batchData = std::make_shared<ppc::io::DataBatch>();
        for (uint32_t col = 0; col < _columnSize; col++)
        {
            parseResult(batchData, _result, rowData, col);
        }
        _queryResult->mutableData().emplace_back(std::move(batchData));
    }
}
