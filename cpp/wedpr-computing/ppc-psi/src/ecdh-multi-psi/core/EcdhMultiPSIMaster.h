#pragma once
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiCache.h"
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIConfig.h"
#include "ppc-psi/src/psi-framework/TaskState.h"

namespace ppc::psi
{
class EcdhMultiPSIMaster : public std::enable_shared_from_this<EcdhMultiPSIMaster>
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIMaster>;
    EcdhMultiPSIMaster(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState);
    virtual ~EcdhMultiPSIMaster()
    {
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        std::vector<bcos::bytes>().swap(m_final_vectors);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the master destroyed") << LOG_KV("taskID", m_taskID);
    }
    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onHandlerIntersectEncryptSetFromCalculator(PSIMessageInterface::Ptr _msg);
    virtual void onHandlerIntersectEncryptSetFromPartner(PSIMessageInterface::Ptr _msg);
    virtual void onHandlerEncryptIntersectionSetFromCalculatorToMaster(
        PSIMessageInterface::Ptr _msg);
    virtual void onHandlerSyncFinalResultToAllPeer(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual void InitAsyncTask(ppc::protocol::Task::ConstPtr _task);
    virtual bool loadCipherDataFinished();
    virtual void onTaskError(std::string&& _error);
    void ConcurrentSTLToCommon(
        tbb::concurrent_map<uint32_t, bcos::bytes> _cMap, std::map<uint32_t, bcos::bytes>& result)
    {
        ConcurrentSTLToCommon(_cMap, 0, _cMap.size(), result);
    };

    void ConcurrentSTLToCommon(tbb::concurrent_map<uint32_t, bcos::bytes> _cMap,
        uint32_t _startIndex, uint32_t _endIndex, std::map<uint32_t, bcos::bytes>& result)
    {
        std::mutex mutex;
        tbb::concurrent_map<uint32_t, bcos::bytes>::const_iterator iter;
        uint32_t index = 0;
        for (iter = _cMap.begin(); iter != _cMap.end(); iter++)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (index < _startIndex)
            {
                index++;
                continue;
            }
            else if (index >= _endIndex)
            {
                break;
            }
            // result.insert(std::make_pair(iter->first, iter->second));
            result.emplace(std::make_pair(iter->first, iter->second));
            index++;
        }
    };

    virtual void trimVector(std::vector<bcos::bytes>& _vectors)
    {
        for (auto it = _vectors.begin(); it != _vectors.end();)
        {
            if (!it->data())
                it = _vectors.erase(it);
            else
                ++it;
        }
    }

    virtual void splitVector(std::vector<bcos::bytes>& _vectors, uint32_t _start, uint32_t _end,
        std::vector<bcos::bytes>& _outVecs)
    {
        uint32_t index = 0;
        for (auto vec : _vectors)
        {
            if (index < _start)
            {
                index++;
                continue;
            }
            else if (index >= _end)
            {
                break;
            }
            _outVecs.push_back(vec);
            index++;
        }
    };

private:
    bool m_syncResult{false};
    ppc::io::DataBatch::Ptr m_originInputs;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;
    std::string m_taskID;
    TaskState::Ptr m_taskState;
    EcdhMultiPSIConfig::Ptr m_config;
    bcos::bytesPointer m_randomB;
    MasterCipherDataCache::Ptr m_masterCipherDataCache;

    mutable boost::shared_mutex x_appendMasterCipherDataFromPartner;
    mutable boost::shared_mutex x_appendMasterCipherDataFromCalculator;

    std::vector<bcos::bytes> m_final_vectors;
    std::map<std::string, uint32_t> m_final_counts;
    mutable bcos::SharedMutex x_final_count;
};
}  // namespace ppc::psi