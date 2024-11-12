/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file Common.h
 * @author: yujiechen
 * @date 2022-12-29
 */
#pragma once
#include "ppc-io/src/FileLineReader.h"
#include "protocol/src/JsonTaskImpl.h"
#include "test-utils/TaskMock.h"
#include <thread>

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::io;

namespace ppc::test
{
template <typename T>
inline void checkTaskPSIResult(T _factory, ppc::protocol::Task::ConstPtr _task,
    uint64_t expectedResultSize, std::vector<std::string> _expectedPSIResult)
{
    auto outputDesc = _task->selfParty()->dataResource()->outputDesc();
    auto reader = _factory->resourceLoader()->loadReader(outputDesc, DataSchema::String, false);
    // get all result
    DataBatch::Ptr result = reader->next(-1);
    // check the result
    std::cout << "### result size:" << (result ? result->size() : 0) << std::endl;
    std::cout << "### expectedResultSize: " << expectedResultSize << std::endl;
    BOOST_CHECK(result->size() == expectedResultSize);
    if (_expectedPSIResult.empty())
    {
        return;
    }
    // Note: the order of the result is not the same with _expectedPSIResult
    std::set<std::string> expectedResult(_expectedPSIResult.begin(), _expectedPSIResult.end());
    for (uint64_t i = 0; i < result->size(); i++)
    {
        BOOST_CHECK(expectedResult.count(result->get<std::string>(i)));
    }
}

//  online psi
template <typename T, typename S>
void testPSI(T _factory, S _server, S _client, ppc::protocol::Task::ConstPtr _serverPsiTask,
    ppc::protocol::Task::ConstPtr _clientPsiTask, bool _expectedSuccess,
    std::vector<std::string> const& _expectedPSIResult, int _expectedErrorCode = 0)
{
    bool serverFinished = false;
    bool clientFinished = false;
    _server->asyncRunTask(_serverPsiTask, [_serverPsiTask, _expectedSuccess, _expectedErrorCode,
                                              &serverFinished](
                                              ppc::protocol::TaskResult::Ptr&& _response) {
        if (_expectedSuccess)
        {
            BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
            BOOST_CHECK(_response->taskID() == _serverPsiTask->id());
            auto result = _response->error();
            BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        }
        else
        {
            BOOST_CHECK(_response->error() != nullptr);
            auto result = _response->error();
            BOOST_CHECK(result != nullptr);
            std::cout << "### response errorCode: " << _response->error()->errorCode() << std::endl;
            std::cout << "### _expectedErrorCode: " << _expectedErrorCode << std::endl;
            BOOST_CHECK(_response->error()->errorCode() == _expectedErrorCode);
        }
        serverFinished = true;
        std::cout << "#### testPSI, the server asyncRunTask success" << std::endl;
    });
    _client->asyncRunTask(_clientPsiTask, [_clientPsiTask, _expectedSuccess, &clientFinished](
                                              ppc::protocol::TaskResult::Ptr&& _response) {
        if (_expectedSuccess)
        {
            BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
            BOOST_CHECK(_response->taskID() == _clientPsiTask->id());
            auto result = _response->error();
            BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        }
        else
        {
            BOOST_CHECK(_response->error() != nullptr);
            auto result = _response->error();
            BOOST_CHECK(result != nullptr);
        }
        clientFinished = true;
        std::cout << "#### testPSI, the client asyncRunTask success" << std::endl;
    });
    // wait for the task finish and check
    while (!clientFinished || !serverFinished || _server->pendingTasksSize() > 0 ||
           _client->pendingTasksSize() > 0)
    {
        _server->executeWorker();
        _client->executeWorker();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    BOOST_CHECK(_client->pendingTasksSize() == 0);
    BOOST_CHECK(_client->lockingResourceSize() == 0);
    BOOST_CHECK(_server->pendingTasksSize() == 0);
    BOOST_CHECK(_server->lockingResourceSize() == 0);
    if (_expectedSuccess)
    {
        checkTaskPSIResult(_factory, _clientPsiTask, _expectedPSIResult.size(), _expectedPSIResult);
    }
}

}  // namespace ppc::test