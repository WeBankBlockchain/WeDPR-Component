#pragma once
#include "Common.h"
#include "EcdhMultiPSIConfig.h"
#include "core/EcdhMultiPSICalculator.h"
#include "core/EcdhMultiPSIMaster.h"
#include "core/EcdhMultiPSIPartner.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-psi/src/psi-framework/TaskGuarder.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Worker.h>

#include <memory>
#include <queue>


namespace ppc::psi
{
class EcdhMultiPSIImpl : public std::enable_shared_from_this<EcdhMultiPSIImpl>,
                         public ppc::task::TaskFrameworkInterface,
                         public TaskGuarder,
                         public bcos::Worker
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIImpl>;
    using EcdhMultiPSIMsgQueue = bcos::ConcurrentQueue<front::PPCMessageFace::Ptr>;
    using EcdhMultiPSIMsgQueuePtr = std::shared_ptr<EcdhMultiPSIMsgQueue>;

    EcdhMultiPSIImpl(const EcdhMultiPSIConfig::Ptr& _config, unsigned _idleTimeMs = 0);

    ~EcdhMultiPSIImpl() override = default;

    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _message) override;

    void start() override;
    void stop() override;

    void checkFinishedTask();
    void onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message) override;
    void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer) override;
    void executeWorker() override;


protected:
    virtual void onReceiveRandomA(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveCalCipher(PSIMessageInterface::Ptr _msg);
    virtual void handlerPSIReceiveMessage(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveIntersecCipher(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveMasterCipher(PSIMessageInterface::Ptr _msg);
    virtual void onReceiveCipherFromPartner(PSIMessageInterface::Ptr _msg);
    virtual void onReceivePSIResult(PSIMessageInterface::Ptr _msg);

    EcdhMultiPSICalculator::Ptr findCalculator(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_calculators);
        auto it = m_calculators.find(_taskID);
        if (it != m_calculators.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void addCalculator(EcdhMultiPSICalculator::Ptr _calculator)
    {
        bcos::WriteGuard l(x_calculators);
        m_calculators[_calculator->taskID()] = _calculator;
    }

    void removeCalculator(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_calculators);
        auto it = m_calculators.find(_taskID);
        if (it != m_calculators.end())
        {
            m_calculators.erase(it);
        }
    }

    EcdhMultiPSIPartner::Ptr findPartner(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_partners);
        auto it = m_partners.find(_taskID);
        if (it != m_partners.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void addPartner(EcdhMultiPSIPartner::Ptr _partner)
    {
        bcos::WriteGuard l(x_partners);
        m_partners[_partner->taskID()] = _partner;
    }

    void removePartner(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_partners);
        auto it = m_partners.find(_taskID);
        if (it != m_partners.end())
        {
            m_partners.erase(it);
        }
    }

    EcdhMultiPSIMaster::Ptr findMaster(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_masters);
        auto it = m_masters.find(_taskID);
        if (it != m_masters.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void addMaster(EcdhMultiPSIMaster::Ptr _master)
    {
        bcos::WriteGuard l(x_masters);
        m_masters[_master->taskID()] = _master;
    }

    void removeMaster(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_partners);
        auto it = m_masters.find(_taskID);
        if (it != m_masters.end())
        {
            m_masters.erase(it);
        }
    }

private:
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signal);
        m_signal.wait_for(l, boost::chrono::milliseconds(5));
    }

    void wakeupWorker() { m_signal.notify_all(); }

    const int c_popWaitMs = 5;

    // at most support 63 peers
    const int c_max_peer_size = 63;

    EcdhMultiPSIConfig::Ptr m_config;
    TaskState::Ptr m_taskMultiState;
    EcdhMultiPSIMsgQueuePtr m_msgQueue;
    boost::condition_variable m_signal;
    boost::mutex x_signal;

    std::unordered_map<std::string, EcdhMultiPSICalculator::Ptr> m_calculators;
    mutable bcos::SharedMutex x_calculators;

    std::unordered_map<std::string, EcdhMultiPSIPartner::Ptr> m_partners;
    mutable bcos::SharedMutex x_partners;

    std::unordered_map<std::string, EcdhMultiPSIMaster::Ptr> m_masters;
    mutable bcos::SharedMutex x_masters;
};
}  // namespace ppc::psi