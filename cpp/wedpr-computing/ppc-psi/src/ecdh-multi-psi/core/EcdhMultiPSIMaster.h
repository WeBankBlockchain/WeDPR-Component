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
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MASTER_LOG(INFO) << LOG_DESC("the master destroyed") << LOG_KV("taskID", m_taskID);
    }
    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onReceiveCalCipher(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveCipherFromPartner(PSIMessageInterface::Ptr _msg);
    virtual void blindData();
    virtual void onReceivePSIResult(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual void initTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onTaskError(std::string&& _error);

private:
    bool m_syncResult{false};
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;
    std::string m_taskID;
    TaskState::Ptr m_taskState;
    EcdhMultiPSIConfig::Ptr m_config;
    bcos::bytesPointer m_randomB;
    MasterCache::Ptr m_masterCache;

    mutable bcos::Mutex m_mutex;
};
}  // namespace ppc::psi