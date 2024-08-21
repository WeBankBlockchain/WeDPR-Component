/**
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
 * @file FileTool.cpp
 * @author: shawnhe
 * @date 2022-12-19
 */

#pragma once

#include <err.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/exception/diagnostic_information.hpp>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ppc::test
{
inline void writeByMmap(
    const std::string& _path, const std::shared_ptr<std::vector<std::string>>& _buffer)
{
    int fd = open(_path.c_str(), O_RDWR | O_CREAT, (mode_t)0600);
    if (fd == -1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("open file error"));
    }

    uint64_t textSize = _buffer->size() * (64);

    // update the size of file
    ftruncate(fd, textSize);

    char* map = (char*)mmap(nullptr, textSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        BOOST_THROW_EXCEPTION(std::runtime_error("mmap file to memory error"));
    }

    uint64_t count = 0;

    for (std::string line : *_buffer)
    {
        for (char c : line)
        {
            map[count++] = c;
        }
        map[count++] = '\n';
    }

    ftruncate(fd, count);

    // write it now to disk
    if (msync(map, count, MS_SYNC) == -1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("mmap file to disk error"));
    }

    munmap(map, count);

    close(fd);
}

inline void prepareInputs(const std::string& _path1, uint32_t _size1, const std::string& _path2,
    uint32_t _size2, uint32_t _common)
{
    assert(_size1 >= _common && _size2 >= _common);

    uint32_t start1 = 100000, end1 = 100000 + _size1;
    auto input1 = std::make_shared<std::vector<std::string>>();
    input1->reserve(_size1);
    for (uint i = start1; i < end1; i++)
    {
        input1->push_back(std::to_string(i) + "\r");
    }
    writeByMmap(_path1, input1);

    uint32_t start2 = end1 - _common, end2 = end1 - _common + _size2;
    auto input2 = std::make_shared<std::vector<std::string>>();
    input2->reserve(_size2);
    for (uint i = start2; i < end2; i++)
    {
        input2->push_back(std::to_string(i));
    }
    writeByMmap(_path2, input2);
}

inline void prepareInputs(const std::string& _path, uint32_t _size)
{
    uint32_t start = 100000, end = 100000 + _size;
    auto input = std::make_shared<std::vector<std::string>>();
    input->reserve(_size);
    for (uint i = start; i < end; i++)
    {
        input->push_back(std::to_string(i));
    }
    writeByMmap(_path, input);
}

inline void prepareItemsAndLabels(const std::string& _path, uint32_t _size)
{
    uint32_t start = 100000, end = 100000 + _size;
    auto input = std::make_shared<std::vector<std::string>>();
    input->reserve(_size);
    for (uint i = start; i < end; i++)
    {
        input->push_back(
            std::to_string(i) + "," + std::to_string(i) + std::to_string(i) + std::to_string(i));
    }
    writeByMmap(_path, input);
}

}  // namespace ppc::test
