#pragma once
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiCache.h"
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIConfig.h"
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <tbb/tbb.h>

namespace ppc::psi
{
class EcdhMultiPSICalculator : public std::enable_shared_from_this<EcdhMultiPSICalculator>
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSICalculator>;

    EcdhMultiPSICalculator(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState);

    virtual ~EcdhMultiPSICalculator()
    {
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_CAL_LOG(INFO) << LOG_DESC("the calculator destroyed") << LOG_KV("taskID", m_taskID);
    }

    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onReceiveIntersecCipher(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveMasterCipher(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual bcos::bytes generateRandomA(std::string _taskID);
    virtual void initTask(ppc::protocol::Task::ConstPtr _task);
    virtual void blindData(std::string _taskID, bcos::bytes _randA);
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


private:
    bool m_syncResult{false};
    EcdhMultiPSIConfig::Ptr m_config;
    bcos::bytes m_randomA;
    std::string m_taskID;
    TaskState::Ptr m_taskState;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;
    std::vector<bcos::bytes> m_finalResults;
    CalculatorCache::Ptr m_calculatorCache;

    mutable bcos::Mutex m_mutex;
};
}  // namespace ppc::psi