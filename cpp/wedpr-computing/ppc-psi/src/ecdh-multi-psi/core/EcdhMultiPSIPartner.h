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
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_PARTNER_LOG(INFO) << LOG_DESC("the partner destroyed") << LOG_KV("taskID", m_taskID);
    }

    virtual void onReceiveRandomA(bcos::bytesPointer _randA);
    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onReceivePSIResult(PSIMessageInterface::Ptr _msg);

    const std::string& taskID() const { return m_taskID; }

protected:
    virtual void initTask(ppc::protocol::Task::ConstPtr _task);
    virtual void onTaskError(std::string&& _error);

private:
    bool m_syncResult{false};
    EcdhMultiPSIConfig::Ptr m_config;
    TaskState::Ptr m_taskState;
    std::string m_taskID;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_calculatorParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_partnerParties;
    std::map<std::string, ppc::protocol::PartyResource::Ptr> m_masterParties;

    mutable bcos::Mutex m_mutex;
};
}  // namespace ppc::psi