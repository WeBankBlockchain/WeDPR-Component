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
 * @file CEMService.cpp
 * @author: caryliao
 * @date 2022-11-04
 */

#include "CEMService.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include "ppc-storage/src/FileStorageFactoryImpl.h"
#include "wedpr_ffi_c_equality.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <stdlib.h>
#include <tbb/parallel_for.h>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace ppc;
using namespace ppc::cem;
using namespace bcos;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::tools;
using namespace ppc::storage;

void CEMService::doCipherTextEqualityMatch(const Json::Value::Members& fieldNames,
    const std::vector<std::string>& fieldValues, LineReader::Ptr lineReader,
    Json::Value& matchCount)
{
    auto fieldNameLine = (lineReader->next(1))->get<std::string>(0);
    std::vector<std::string> fileFieldNames;
    // obtain the fieldName
    boost::split(fileFieldNames, fieldNameLine, boost::is_any_of(","));
    std::vector<uint32_t> matchFieldColumnIndexs;
    // obtain the field that need to be matched, put it into matchFieldColumnIndexs
    for (size_t i = 0; i < fileFieldNames.size(); i++)
    {
        for (auto fieldName : fieldNames)
        {
            auto fileFieldName = fileFieldNames.at(i);
            boost::algorithm::trim(fieldName);
            boost::algorithm::trim(fileFieldName);
            if (fieldName == fileFieldName)
            {
                matchFieldColumnIndexs.emplace_back(i);
            }
        }
    }
    uint64_t lineSize = 0;
    for (auto fieldName : fieldNames)
    {
        matchCount[fieldName] = 0;
    }
    // record the match-result
    std::vector<std::atomic<uint64_t>> matchCountResult(matchFieldColumnIndexs.size());
    int64_t readPerBatchLines = m_cemConfig.readPerBatchLines;
    while (true)
    {
        // batch read dataset line
        auto dataBatch = lineReader->next(readPerBatchLines);
        if (!dataBatch)
        {
            break;
        }
        lineSize += dataBatch->size();
        // paraller for handle ciphtertext match in dataset
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0U, dataBatch->size()), [&](auto const& range) {
                for (auto i = range.begin(); i < range.end(); i++)
                {
                    auto fileLine = dataBatch->get<std::string>(i);
                    // erase windows \r character
                    fileLine.erase(
                        std::remove(fileLine.begin(), fileLine.end(), '\r'), fileLine.end());
                    std::vector<std::string> fileFieldValues;
                    boost::split(fileFieldValues, fileLine, boost::is_any_of(","));
                    tbb::parallel_for(tbb::blocked_range<size_t>(0U, matchFieldColumnIndexs.size()),
                        [&](auto const& range) {
                            for (auto j = range.begin(); j < range.end(); j++)
                            {
                                // auto startT = utcSteadyTime();
                                auto decodedRequestValue = fromHexString(fieldValues.at(j));
                                auto decodedFileValue =
                                    fromHexString(fileFieldValues.at(matchFieldColumnIndexs.at(j)));
                                CInputBuffer inputBuffer1{
                                    (const char*)decodedRequestValue->data(), CIPHERTEXT_LEN};
                                CInputBuffer inputBuffer2{
                                    (const char*)decodedFileValue->data(), CIPHERTEXT_LEN};
                                auto matchResult = wedpr_pairing_bls128_equality_test(
                                    &inputBuffer1, &inputBuffer2);
                                // CEM_LOG(INFO) << LOG_DESC("match once ok")
                                //              << LOG_KV("timecost(ms)", utcSteadyTime() - startT);
                                if (matchResult == WEDPR_SUCCESS)
                                {
                                    (matchCountResult[j]).fetch_add(1, std::memory_order_relaxed);
                                }
                            }
                        });
                }
            });
    }
    // assign the result
    for (uint64_t i = 0; i < matchFieldColumnIndexs.size(); i++)
    {
        auto fieldName = fileFieldNames.at(matchFieldColumnIndexs.at(i));
        boost::algorithm::trim(fieldName);
        matchCount[fieldName] = matchCountResult[i].load();
    }
    CEM_LOG(INFO) << LOG_DESC("match dataset file ok") << LOG_KV("file lines", lineSize);
}

void CEMService::makeCiphertextEqualityMatch(
    Json::Value const& request, Json::Value& response, DataResourceType _type)
{
    auto startT = utcSteadyTime();
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, request.size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            auto datasetId = request[(Json::ArrayIndex)i]["dataset_id"].asString();
            auto matchField = request[(Json::ArrayIndex)i]["match_field"];
            Json::Value result;
            result["dataset_id"] = datasetId;
            auto fieldNames = matchField.getMemberNames();
            std::vector<std::string> fieldValues;
            for (auto fieldName : fieldNames)
            {
                const std::string fieldValue = matchField[fieldName].asString();
                fieldValues.emplace_back(fieldValue);
            }
            Json::Value matchCount;
            auto linereader = initialize_lineReader(datasetId, _type);
            doCipherTextEqualityMatch(fieldNames, fieldValues, linereader, matchCount);
            result["match_count"] = matchCount;
            response[(Json::ArrayIndex)i] = result;
        }
    });
    CEM_LOG(INFO) << LOG_DESC("ciphertext equality match request handle ok")
                  << LOG_KV("timecost(ms)", utcSteadyTime() - startT);
}

void CEMService::makeCiphertextEqualityMatchRpc(Json::Value const& request, RespFunc func)
{
    Json::Value response(Json::arrayValue);
    response.resize((Json::ArrayIndex)request.size());
    makeCiphertextEqualityMatch(request, response, DataResourceType::HDFS);
    func(nullptr, std::move(response));
}

void CEMService::doEncryptDataset(LineReader::Ptr lineReader, LineWriter::Ptr lineWriter)
{
    try
    {
        auto fieldNameLine = (lineReader->next(1))->get<std::string>(0);
        auto dataBatchFieldName = std::make_shared<DataBatch>();
        // erase windows \r character
        fieldNameLine.erase(
            std::remove(fieldNameLine.begin(), fieldNameLine.end(), '\r'), fieldNameLine.end());
        dataBatchFieldName->append(fieldNameLine);
        lineWriter->writeLine(dataBatchFieldName, DataSchema::String, "\n");
        uint64_t lineSize = 0;
        int64_t readPerBatchLines = m_cemConfig.readPerBatchLines;
        while (true)
        {
            // batch read dataset line
            auto dataBatch = lineReader->next(readPerBatchLines);
            if (!dataBatch)
            {
                break;
            }
            lineSize += dataBatch->size();
            auto ciphertextFileLines = std::make_shared<DataBatch>();
            ciphertextFileLines->resize(dataBatch->size());
            // parallel for handle encrypt dataset
            tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataBatch->size()), [&](auto const&
                                                                                         range) {
                for (auto i = range.begin(); i < range.end(); i++)
                {
                    auto fileLine = dataBatch->get<std::string>(i);
                    // erase windows \r character
                    fileLine.erase(
                        std::remove(fileLine.begin(), fileLine.end(), '\r'), fileLine.end());
                    std::vector<std::string> fileFieldValues;
                    boost::split(fileFieldValues, fileLine, boost::is_any_of(","));
                    std::ostringstream ciphertextFileLine;
                    for (uint64_t i = 0; i < fileFieldValues.size(); i++)
                    {
                        // id in index 0, need not be encrypted
                        auto fieldValue = fileFieldValues.at(i);
                        boost::algorithm::trim(fieldValue);
                        if (i == 0)
                        {
                            ciphertextFileLine << fieldValue;
                        }
                        else
                        {
                            // auto startT = utcSteadyTime();
                            const char* plaintext = fieldValue.data();
                            CInputBuffer inputBuffer{plaintext, fieldValue.size()};
                            auto ciphertext = std::make_shared<bytes>();
                            ciphertext->resize(CIPHERTEXT_LEN);
                            COutputBuffer outputBuffer{(char*)ciphertext->data(), CIPHERTEXT_LEN};
                            auto encryptResult =
                                wedpr_pairing_bls128_encrypt_message(&inputBuffer, &outputBuffer);
                            //              << LOG_KV("timecost(ms)", utcSteadyTime() - startT);
                            if (encryptResult == WEDPR_SUCCESS)
                            {
                                auto ciphertextHexStr = *toHexString(*ciphertext);
                                ciphertextFileLine << "," << ciphertextHexStr;
                            }
                            else
                            {
                                BOOST_THROW_EXCEPTION(
                                    OpenDatasetFileFailException()
                                    << errinfo_comment("encrypt dataset file fail"));
                            }
                        }
                    }
                    ciphertextFileLines->set(i, ciphertextFileLine.str());
                }
            });
            lineWriter->writeLine(ciphertextFileLines, DataSchema::String, "\n");
        }
        lineWriter->close();
        CEM_LOG(INFO) << LOG_DESC("encrypt dataset file ok") << LOG_KV("file lines", lineSize);
    }
    catch (const char*& e)
    {
        CEM_LOG(INFO) << LOG_KV("doEncryptDataset", e);
    }
}

void CEMService::encryptDataset(Json::Value const& request, Json::Value& response)
{
    // parallel for handle dataset
    auto startT = utcSteadyTime();
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, request.size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            const std::string datasetId = request[(Json::ArrayIndex)i].asString();
            auto linereader = initialize_lineReader(datasetId, DataResourceType::HDFS);
            auto linewriter = initialize_lineWriter(datasetId, DataResourceType::HDFS);
            doEncryptDataset(linereader, linewriter);
            renameSource(datasetId, DataResourceType::HDFS);
            response[(Json::ArrayIndex)i] = datasetId + m_cemConfig.ciphertextSuffix;
        }
    });
    CEM_LOG(INFO) << LOG_DESC("encrypt dataset request handle ok")
                  << LOG_KV("timecost(ms)", utcSteadyTime() - startT);
}

void CEMService::encryptDatasetRpc(Json::Value const& request, RespFunc func)
{
    Json::Value response;
    encryptDataset(request, response);
    func(nullptr, std::move(response));
}

LineReader::Ptr CEMService::initialize_lineReader(
    const std::string& _datasetId, DataResourceType _type)
{
    std::string file_to = m_cemConfig.datasetFilePath + "/" + _datasetId;
    auto factory = std::make_shared<FileStorageFactoryImpl>();
    auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
        nullptr, nullptr, nullptr, nullptr, factory, nullptr);
    const auto dataResource = std::make_shared<DataResource>();
    dataResource->setResourceID(_datasetId);
    const auto dataResourceDesc = std::make_shared<DataResourceDesc>();
    dataResourceDesc->setType((int)_type);
    if ((int)_type == (int)DataResourceType::HDFS)
    {
        file_to = m_cemConfig.datasetHDFSPath + "/" + _datasetId;
        dataResourceDesc->setFileStorageConnectOption(m_storageConfig.fileStorageConnectionOpt);
    }
    else if ((int)_type == (int)DataResourceType::FILE)
    {
        file_to = m_cemConfig.datasetFilePath + "/" + _datasetId;
    }
    dataResourceDesc->setPath(file_to);
    dataResource->setDesc(dataResourceDesc);
    return dataResourceLoader->loadReader(dataResource->desc());
}

LineWriter::Ptr CEMService::initialize_lineWriter(
    const std::string& _datasetId, DataResourceType _type)
{
    std::string file_to =
        m_cemConfig.datasetFilePath + "/" + _datasetId + m_cemConfig.ciphertextSuffix;
    auto factory = std::make_shared<FileStorageFactoryImpl>();
    auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
        nullptr, nullptr, nullptr, nullptr, factory, nullptr);
    const auto dataResource = std::make_shared<DataResource>();
    dataResource->setResourceID(_datasetId);
    const auto dataResourceDesc = std::make_shared<DataResourceDesc>();
    dataResourceDesc->setType((int)_type);
    if ((int)_type == (int)DataResourceType::HDFS)
    {
        file_to = m_cemConfig.datasetHDFSPath + "/" + _datasetId + m_cemConfig.ciphertextSuffix;
        dataResourceDesc->setFileStorageConnectOption(m_storageConfig.fileStorageConnectionOpt);
    }
    else if ((int)_type == (int)DataResourceType::FILE)
    {
        file_to = m_cemConfig.datasetFilePath + "/" + _datasetId + m_cemConfig.ciphertextSuffix;
    }
    dataResourceDesc->setPath(file_to);
    dataResource->setOutputDesc(dataResourceDesc);
    return dataResourceLoader->loadWriter(dataResource->outputDesc());
}

void CEMService::renameSource(const std::string& _datasetId, DataResourceType _type)
{
    if ((int)_type == (int)DataResourceType::HDFS)
    {
        std::string from_file =
            m_cemConfig.datasetHDFSPath + "/" + _datasetId + m_cemConfig.ciphertextSuffix;
        std::string to_file = m_cemConfig.datasetHDFSPath + "/" + _datasetId;
        auto factory = std::make_shared<FileStorageFactoryImpl>();
        auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
            nullptr, nullptr, nullptr, nullptr, factory, nullptr);
        const auto dataResource = std::make_shared<DataResource>();
        dataResource->setResourceID(_datasetId);
        const auto dataResourceDesc = std::make_shared<DataResourceDesc>();
        dataResourceDesc->setType((int)_type);
        dataResourceDesc->setFileStorageConnectOption(m_storageConfig.fileStorageConnectionOpt);
        dataResourceDesc->setPath(from_file);
        dataResource->setOutputDesc(dataResourceDesc);
        dataResourceLoader->renameResource(dataResource->outputDesc(), to_file);
    }
    else if ((int)_type == (int)DataResourceType::FILE)
    {
        std::string from_file =
            m_cemConfig.datasetFilePath + "/" + _datasetId + m_cemConfig.ciphertextSuffix;
        std::string to_file = m_cemConfig.datasetFilePath + "/" + _datasetId;
        if (boost::filesystem::exists(from_file))
        {
            boost::filesystem::remove(from_file);
        }
        boost::filesystem::rename(from_file, to_file);
    }
}


void CEMService::setCEMConfig(CEMConfig const& cemConfig)
{
    m_cemConfig = cemConfig;
}

void CEMService::setStorageConfig(StorageConfig const& storageConfig)
{
    m_storageConfig = storageConfig;
}