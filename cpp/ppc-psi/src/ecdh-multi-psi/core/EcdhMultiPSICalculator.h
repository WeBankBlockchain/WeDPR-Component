#pragma once
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiCache.h"
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIConfig.h"
#include "ppc-psi/src/psi-framework/TaskState.h"

namespace ppc::psi
{
class EcdhMultiPSICalculator : public std::enable_shared_from_this<EcdhMultiPSICalculator>
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSICalculator>;

    EcdhMultiPSICalculator(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState);

    virtual ~EcdhMultiPSICalculator()
    {
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the calculator destroyed") << LOG_KV("taskID", m_taskID);
    }

    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onHandlerIntersectEncryptSetToCalculator(PSIMessageInterface::Ptr _msg);
    virtual void onHandlerEncryptSetToCalculator(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual bcos::bytes generateRandomA(std::string _taskID);
    virtual void InitAsyncTask(ppc::protocol::Task::ConstPtr _task);
    virtual void computeAndEncryptSet(std::string _taskID, bcos::bytes _randA);
    virtual void syncResultToAllPeers();
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
            result.emplace(std::make_pair(iter->first, iter->second));
            // result.insert(std::make_pair(iter->first, iter->second));
            index++;
        }
    };

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
    EcdhMultiPSIConfig::Ptr m_config;
    bcos::bytes m_randomA;
    std::string m_taskID;
    TaskState::Ptr m_taskState;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;
    std::vector<bcos::bytes> m_finalResults;
    CalculatorCipherDataCache::Ptr m_calculatorCipherDataCache;
};
}  // namespace ppc::psi