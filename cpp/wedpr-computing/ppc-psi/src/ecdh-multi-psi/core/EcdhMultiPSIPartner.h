#pragma once
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIConfig.h"
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <string.h>

namespace ppc::psi
{
class EcdhMultiPSIPartner : public std::enable_shared_from_this<EcdhMultiPSIPartner>
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIPartner>;

    EcdhMultiPSIPartner(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState);

    virtual ~EcdhMultiPSIPartner()
    {
        std::vector<bcos::bytes>().swap(m_final_vectors);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the partner destroyed") << LOG_KV("taskID", m_taskID);
    }

    virtual void oncomputeAndEncryptSet(bcos::bytesPointer _randA);
    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onHandlerSyncFinalResultToAllPeer(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual void InitAsyncTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onTaskError(std::string&& _error);

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
    EcdhMultiPSIConfig::Ptr m_config;
    TaskState::Ptr m_taskState;
    std::string m_taskID;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;

    mutable bcos::SharedMutex x_final_count;
    std::vector<bcos::bytes> m_final_vectors;
    std::map<std::string, uint32_t> m_final_counts;
};
}  // namespace ppc::psi