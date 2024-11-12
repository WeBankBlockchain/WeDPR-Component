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
 * @file CommandHelper.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "CommandHelper.h"
#include <include/BuildInfo.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

using namespace ppc;

void ppc::printVersion(std::string const& binaryName)
{
    std::cout << binaryName << " Version : " << PPC_PROJECT_VERSION << std::endl;
    std::cout << "Build Time         : " << PPC_BUILD_TIME << std::endl;
    std::cout << "Build Type         : " << PPC_BUILD_PLATFORM << "/" << PPC_BUILD_TYPE
              << std::endl;
    std::cout << "Git Branch         : " << PPC_BUILD_BRANCH << std::endl;
    std::cout << "Git Commit         : " << PPC_COMMIT_HASH << std::endl;
}

CommandLineParam ppc::initCommandLine(std::string const& binaryName, int argc, const char* argv[])
{
    boost::program_options::options_description main_options("Usage of PPC");
    main_options.add_options()("help,h", "print help information")("version,v", "version of PPC")(
        "config,c", boost::program_options::value<std::string>(), "./config.ini");
    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, main_options), vm);
    }
    catch (...)
    {
        printVersion(binaryName);
    }
    /// help information
    if (vm.count("help") || vm.count("h"))
    {
        std::cout << main_options << std::endl;
        exit(0);
    }
    /// version information
    if (vm.count("version") || vm.count("v"))
    {
        printVersion(binaryName);
        exit(0);
    }
    std::string configPath("./config.ini");
    if (vm.count("config"))
    {
        configPath = vm["config"].as<std::string>();
    }
    if (vm.count("c"))
    {
        configPath = vm["c"].as<std::string>();
    }
    // std::ifstream configFile(configPath);
    // if (!configFile)
    if (!boost::filesystem::exists(configPath))
    {
        std::cout << "config \'" << configPath << "\' not found!";
        exit(0);
    }
    return ppc::CommandLineParam{configPath};
}
