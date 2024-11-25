/**
 *  Copyright (C) 2024 WeDPR.
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
 * @file MemInfo.h
 * @author: shawnhe
 * @date 2024-03-04
 */

#pragma once

#ifdef __linux__
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace ppc::tools
{
inline bool hasAvailableMem(uint32_t _minNeedMemoryGB)
{
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open())
    {
        std::cerr << "Failed to open /proc/meminfo!" << std::endl;
        return -1;
    }

    std::string line;
    long long availableMem = 0;

    while (std::getline(meminfo, line))
    {
        if (line.find("MemAvailable:") != std::string::npos)
        {
            sscanf(line.c_str(), "MemAvailable: %lld kB", &availableMem);
            break;
        }
    }

    meminfo.close();

    auto availableMemGB = availableMem / (1024 * 1024);  // Convert to GB
    return availableMemGB >= _minNeedMemoryGB;
}
}  // namespace ppc::tools
#endif
