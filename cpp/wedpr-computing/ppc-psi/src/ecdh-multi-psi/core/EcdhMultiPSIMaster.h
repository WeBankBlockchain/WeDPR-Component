#pragma once
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiCache.h"
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIConfig.h"
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <tbb/tbb.h>

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
        ECDH_MASTER_LOG(INFO) << LOG_DESC("the master destroyed") << LOG_KV("taskID", m_taskID);
    }
    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onHandlerIntersectEncryptSetFromCalculator(PSIMessageInterface::Ptr _msg);
    virtual void onHandlerIntersectEncryptSetFromPartner(PSIMessageInterface::Ptr _msg);
    virtual void blindData();
    virtual void onReceivePSIResult(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual void encAndSendIntersectionData();

    virtual void initTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onTaskError(std::string&& _error);
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
    MasterCache::Ptr m_masterCache;

    mutable boost::SharedMutex x_addCalculatorCipher;
};
}  // namespace ppc::psi