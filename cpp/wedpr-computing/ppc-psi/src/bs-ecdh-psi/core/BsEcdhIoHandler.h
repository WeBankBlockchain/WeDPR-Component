/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file BsEcdhIoHandler.h
 * @author: shawnhe
 * @date 2023-09-22
 */

#pragma once

#include <utility>

#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/io/LineReader.h"
#include "ppc-psi/src/bs-ecdh-psi/Common.h"

namespace ppc::psi
{
class BsEcdhIoHandler
{
public:
    using Ptr = std::shared_ptr<BsEcdhIoHandler>;

    BsEcdhIoHandler(std::string _taskID, ppc::io::LineReader::Ptr _reader,
        ppc::io::LineWriter::Ptr _resultWriter, ppc::io::LineWriter::Ptr _indexWriter,
        ppc::io::LineWriter::Ptr _evidenceWriter)
      : m_taskID(std::move(_taskID)),
        m_reader(std::move(_reader)),
        m_resultWriter(std::move(_resultWriter)),
        m_indexWriter(std::move(_indexWriter)),
        m_evidenceWriter(std::move(_evidenceWriter))
    {}

    virtual ~BsEcdhIoHandler() = default;

    [[nodiscard]] std::string taskID() const { return m_taskID; }

    [[nodiscard]] ppc::io::LineReader::Ptr reader() const { return m_reader; }

    [[nodiscard]] ppc::io::LineWriter::Ptr resultWriter() const { return m_resultWriter; }

    [[nodiscard]] ppc::io::LineWriter::Ptr indexWriter() const { return m_indexWriter; }

    [[nodiscard]] ppc::io::LineWriter::Ptr evidenceWriter() const { return m_evidenceWriter; }

    io::DataBatch::Ptr loadInputs()
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start loading inputs") << LOG_KV("taskID", m_taskID);
        int64_t nextParam = m_reader->type() == protocol::DataResourceType::MySQL ? 0 : -1;
        auto data = m_reader->next(nextParam, io::DataSchema::String);
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish loading inputs") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("inputsSize", data->size());
        return data;
    }

    protocol::FileInfo::Ptr saveResults(const std::unordered_set<std::string>& _results)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start saving results") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("resultsSize", _results.size());
        writeByDataBatch(m_resultWriter, _results);
        m_resultWriter->flush();
        m_resultWriter->close();
        // upload
        m_resultWriter->upload();
        m_resultWriter->clean();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish saving results") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("resultsSize", _results.size());
        return m_resultWriter->fileInfo();
    }

    protocol::FileInfo::Ptr saveIndexes(const io::DataBatch::Ptr& _indexes)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start saving indexes") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("indexesSize", _indexes->size());
        m_indexWriter->writeLine(_indexes, io::DataSchema::String, "\n");

        m_indexWriter->flush();
        m_indexWriter->close();
        // upload
        m_indexWriter->upload();
        m_indexWriter->clean();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish saving indexes") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("indexesSize", _indexes->size());
        return m_indexWriter->fileInfo();
    }

    void appendEvidence(const std::string& _tag, const std::string& _evidence)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("append evidence") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("tag", _tag);
        if (!m_evidenceWriter)
        {
            return;
        }
        io::DataBatch::Ptr dataBatch = std::make_shared<io::DataBatch>();
        dataBatch->append(_tag);
        dataBatch->append(_evidence);
        m_evidenceWriter->writeLine(dataBatch, io::DataSchema::String, "\n");
        m_evidenceWriter->flush();
    }

    void appendEvidences(const std::string& _tag, const std::vector<std::string>& _evidences)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start appending evidences") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("tag", _tag) << LOG_KV("evidencesSize", _evidences.size());
        if (!m_evidenceWriter)
        {
            return;
        }
        io::DataBatch::Ptr dataBatch = std::make_shared<io::DataBatch>();
        dataBatch->append(_tag);
        m_evidenceWriter->writeLine(dataBatch, io::DataSchema::String, "\n");
        m_evidenceWriter->flush();
        writeByDataBatch(m_evidenceWriter, _evidences);
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish appending evidences")
                              << LOG_KV("taskID", m_taskID) << LOG_KV("tag", _tag)
                              << LOG_KV("evidencesSize", _evidences.size());
    }

    protocol::FileInfo::Ptr uploadEvidences()
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start uploading evidences")
                              << LOG_KV("taskID", m_taskID);
        if (!m_evidenceWriter)
        {
            return nullptr;
        }
        m_evidenceWriter->flush();
        m_evidenceWriter->close();
        // upload
        m_evidenceWriter->upload();
        m_evidenceWriter->clean();
        return m_evidenceWriter->fileInfo();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish uploading evidences")
                              << LOG_KV("taskID", m_taskID);
    }

    void clean()
    {
        if (m_reader)
        {
            m_reader->clean();
        }
        if (m_resultWriter)
        {
            m_resultWriter->clean();
        }
        if (m_indexWriter)
        {
            m_indexWriter->clean();
        }
        if (m_evidenceWriter)
        {
            m_evidenceWriter->clean();
        }
    }

    template <typename Container>
    static void writeByDataBatch(const ppc::io::LineWriter::Ptr& _writer, const Container& _data)
    {
        io::DataBatch::Ptr dataBatch = std::make_shared<io::DataBatch>();
        uint32_t count = 0;
        for (const auto& line : _data)
        {
            dataBatch->append<std::string>(line);
            count++;
            if (count == 1000000)
            {
                _writer->writeLine(dataBatch, io::DataSchema::String, "\n");
                _writer->flush();
                dataBatch = std::make_shared<io::DataBatch>();
                count = 0;
            }
        }
        if (count > 0)
        {
            _writer->writeLine(dataBatch, io::DataSchema::String, "\n");
            _writer->flush();
        }
    }

private:
    std::string m_taskID;
    ppc::io::LineReader::Ptr m_reader;
    ppc::io::LineWriter::Ptr m_resultWriter;
    ppc::io::LineWriter::Ptr m_indexWriter;
    ppc::io::LineWriter::Ptr m_evidenceWriter;
};

}  // namespace ppc::psi
