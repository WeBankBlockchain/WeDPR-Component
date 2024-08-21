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
 * @file MySQLStorage.h
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include "Common.h"
#include "ppc-framework/storage/SQLStorage.h"
#include <bcos-utilities/Timer.h>
#include <mysqlx/xapi.h>
#include <sstream>

namespace ppc::storage
{
class MySQLStatement : public Statement
{
public:
    using Ptr = std::shared_ptr<MySQLStatement>;
    MySQLStatement(mysqlx_stmt_t* _statement, mysqlx_session_t* _session)
      : m_statement(_statement), m_session(_session)
    {}

    ~MySQLStatement() override
    {
        // return the session
        if (m_callback)
        {
            m_callback(m_session);
        }
    }

    void* statementContext() override { return (void*)m_statement; }
    void* takeSession() override
    {
        auto session = m_session;
        m_session = nullptr;
        return (void*)session;
    }

    void setReturnSessionCallback(std::function<void(mysqlx_session_t*)> const& _callback)
    {
        m_callback = _callback;
    }

private:
    mysqlx_stmt_t* m_statement;
    mysqlx_session_t* m_session;
    std::function<void(mysqlx_session_t*)> m_callback;
};

class MySQLResultContext : public SQLResultContext
{
public:
    using Ptr = std::shared_ptr<MySQLResultContext>;
    MySQLResultContext(mysqlx_result_t* _result) : m_result(_result) {}
    ~MySQLResultContext() override
    {
        if (m_result)
        {
            mysqlx_free(m_result);
        }
        if (m_callback)
        {
            m_callback(m_session);
        }
    }

    // get the sql-result
    void* result() override { return (void*)m_result; }
    virtual void setSession(void* _session) { m_session = (mysqlx_session_t*)_session; }

    void setReturnSessionCallback(std::function<void(mysqlx_session_t*)> const& _callback)
    {
        m_callback = _callback;
    }

private:
    mysqlx_result_t* m_result;
    mysqlx_session_t* m_session;
    std::function<void(mysqlx_session_t*)> m_callback;
};

class MySQLStorage : public SQLStorage
{
public:
    // TODO: configure the session-pool-size
    using Ptr = std::shared_ptr<MySQLStorage>;
    MySQLStorage(
        ppc::protocol::SQLConnectionOption::Ptr const& _opt, uint16_t _sessionPoolSize = 16);
    ~MySQLStorage() override
    {
        if (m_timer)
        {
            m_timer->stop();
        }
        destroyAllSession();
    }

    // use the database
    void useDataBase(const char* _database) override;

    // exec the sql-query-command, throws exception when query-error
    QueryResult::Ptr execQuery(bool _parseByColumn, const char* _command, ...) const override;
    // exec the sql-write-command: insert/update/delete, throws exception when exec-error
    void execCommit(const char* _command, ...) const override;

    std::string const& databaseName() const override { return m_opt->database; }

    Statement::Ptr generateStatement(const char* _command, ...) const override;
    SQLResultContext::Ptr execStatement(Statement::Ptr _statement) const override;
    void appendStatement(Statement::Ptr _statement, ...) const override;

protected:
    virtual void refreshConnection();
    virtual mysqlx_session_t* connectDataBase(const char* _database, bool _disableSSl) const;
    mysqlx_session_t* createSession() const;
    virtual void destroySession(mysqlx_session_t* _session) const
    {
        if (_session)
        {
            // close the session
            mysqlx_session_close(_session);
        }
    }
    // destroy the current session
    virtual void destroyAllSession()
    {
        std::vector<mysqlx_session_t*> sessionPool;
        {
            bcos::WriteGuard l(x_sessionPool);
            sessionPool.swap(m_sessionPool);
        }
        for (auto const& it : sessionPool)
        {
            destroySession(it);
        }
    }

    void parseMetaData(QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result,
        uint32_t _columnSize) const;

    // parse the queried-result by row
    void parseResultByRow(QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result,
        uint32_t _columnSize) const;
    // parse the queried result by column
    void parseResultByColumn(QueryResult::Ptr _queryResult, SQLResultContext::Ptr const& _result,
        uint32_t _columnSize) const;
    // parse the result according to the type
    void parseResult(ppc::io::DataBatch::Ptr _result, SQLResultContext::Ptr const& _mysqlResult,
        mysqlx_row_t* _rowData, uint32_t _col) const;

    template <typename T>
    bool inline appendResult(mysqlx_row_t* _rowData, ppc::io::DataBatch::Ptr _result, int _retCode,
        T&& _element, bool _append = true) const
    {
        bool readFinished = true;
        if (RESULT_ERROR == _retCode)
        {
            std::string errorMsg = mysqlx_error_message(_rowData);
            BOOST_THROW_EXCEPTION(ParseMySQLResultError() << bcos::errinfo_comment(
                                      "parse sql-result error, code:" + std::to_string(_retCode) +
                                      ", error: " + errorMsg));
        }
        // the data has not been read-finished yet
        if (RESULT_MORE_DATA == _retCode)
        {
            readFinished = false;
        }
        if (_append)
        {
            _result->append(std::move(_element));
        }
        return readFinished;
    }

    template <typename... Args>
    void bindStmt(mysqlx_stmt_t* stmt, Args... args) const
    {
        auto result = mysqlx_stmt_bind(stmt, args...);
        if (result != RESULT_OK)
        {
            MYSQL_STORAGE_LOG(ERROR) << LOG_DESC("bindStmt error") << LOG_KV("code", result)
                                     << LOG_KV("msg", mysqlx_error_message(stmt));
            BOOST_THROW_EXCEPTION(
                ExecMySQLError() << bcos::errinfo_comment(
                    "bindStmt error: " + std::string(mysqlx_error_message(stmt))));
        }
    }

    Statement::Ptr generateSQLStatement(const char* _command, va_list& _args) const;
    void appendStatement(Statement::Ptr stmt, va_list& args) const;

    SQLResultContext::Ptr execSQL(const char* _command, va_list& _args) const
    {
        auto statement = generateSQLStatement(_command, _args);
        return execStatement(statement);
    }


    mysqlx_session_t* allocateSession() const
    {
        // Note: can't new multiple mysql-session at the same times
        bcos::WriteGuard l(x_sessionPool);
        // all session has been used-up, allocate a new session
        if (m_sessionPool.empty())
        {
            MYSQL_STORAGE_LOG(DEBUG) << LOG_DESC("allocateSession for the session pool is empty");
            return createSession();
        }
        MYSQL_STORAGE_LOG(DEBUG) << LOG_DESC("allocateSession from the session pool")
                                 << LOG_KV("poolSize", m_sessionPool.size());
        // obtain session from the sessionPool
        auto session = m_sessionPool.back();
        m_sessionPool.pop_back();
        return session;
    }

    // Note: the session can be only returned after the result obtained
    void returnSession(mysqlx_session_t* _session) const
    {
        if (!_session)
        {
            return;
        }
        bcos::UpgradableGuard l(x_sessionPool);
        if (m_sessionPool.size() >= m_sessionPoolSize)
        {
            MYSQL_STORAGE_LOG(DEBUG)
                << LOG_DESC("returnSession: destroy the session for the pool-size over limit")
                << LOG_KV("poolSize", m_sessionPool.size());
            destroySession(_session);
            return;
        }
        bcos::UpgradeGuard ul(l);
        m_sessionPool.emplace_back(_session);
        MYSQL_STORAGE_LOG(DEBUG) << LOG_DESC("returnSession")
                                 << LOG_KV("poolSize", m_sessionPool.size());
    }

    void insertSession(mysqlx_session_t* _session)
    {
        bcos::WriteGuard l(x_sessionPool);
        m_sessionPool.emplace_back(_session);
    }

private:
    ppc::protocol::SQLConnectionOption::Ptr m_opt;
    // TODO: support session-pool
    std::vector<mysqlx_session_t*> mutable m_sessionPool;
    bcos::SharedMutex mutable x_sessionPool;

    uint16_t m_sessionPoolSize = 16;
    std::shared_ptr<bcos::Timer> m_timer;

    int const c_maxElementLen = 65536;
};
}  // namespace ppc::storage