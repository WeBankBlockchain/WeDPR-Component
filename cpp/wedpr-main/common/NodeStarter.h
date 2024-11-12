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
 * @file main.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */

#include <execinfo.h>
#include <libhelper/CommandHelper.h>
#include <libhelper/ExitHandler.h>
#include <signal.h>
#include <stdexcept>
#include <thread>

namespace ppc::node
{
template <typename T>
int startProgram(
    int argc, const char* argv[], std::string const& binaryName, std::shared_ptr<T> starter)
{
    /// set LC_ALL
    setDefaultOrCLocale();
    std::set_terminate([]() {
        std::cout << "terminate handler called, print stacks" << std::endl;
        void* trace_elems[20];
        int trace_elem_count(backtrace(trace_elems, 20));
        char** stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
        for (int i = 0; i < trace_elem_count; ++i)
        {
            std::cout << stack_syms[i] << "\n";
        }
        free(stack_syms);
        std::cout << "terminate handler called, print stack end" << std::endl;
        abort();
    });
    // get datetime and output welcome info
    ppc::ExitHandler exitHandler;
    signal(SIGTERM, &ppc::ExitHandler::exitHandler);
    signal(SIGABRT, &ppc::ExitHandler::exitHandler);
    signal(SIGINT, &ppc::ExitHandler::exitHandler);

    // Note: the initializer must exist in the life time of the whole program
    try
    {
        auto param = ppc::initCommandLine(binaryName, argc, argv);
        starter->init(param.configFilePath);
        starter->start();
    }
    catch (std::exception const& e)
    {
        printVersion(binaryName);
        std::cout << "[" << bcos::getCurrentDateTime() << "] ";
        std::cout << "start " + binaryName + " failed, error:" << boost::diagnostic_information(e)
                  << std::endl;
        return -1;
    }
    printVersion(binaryName);
    std::cout << "[" << bcos::getCurrentDateTime() << "] ";
    std::cout << "The " + binaryName + " is running..." << std::endl;
    while (!exitHandler.shouldExit())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}
}  // namespace ppc::node