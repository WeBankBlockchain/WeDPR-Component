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
 * @file Initializer.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */
#include "Initializer.h"
#include "Common.h"
#include "ProFrontInitializer.h"
#include "ppc-crypto/src/ecc/ECDHCryptoFactoryImpl.h"
#include "ppc-crypto/src/oprf/RA2018Oprf.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-pir/src/OtPIRFactory.h"
#include "ppc-pir/src/OtPIRImpl.h"
#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-psi/src/bs-ecdh-psi/BsEcdhPSIFactory.h"
#include "ppc-psi/src/cm2020-psi/CM2020PSIFactory.h"
#if 0
//TODO: optimize here
#include "ppc-psi/src/ecdh-conn-psi/EcdhConnPSIFactory.h"
#endif
#include "ppc-psi/src/ecdh-multi-psi/EcdhMultiPSIFactory.h"
#include "ppc-psi/src/ecdh-psi/EcdhPSIFactory.h"
#include "ppc-psi/src/labeled-psi/LabeledPSIFactory.h"
#include "ppc-psi/src/ra2018-psi/RA2018PSIFactory.h"
#include <tbb/tbb.h>
#include <thread>

using namespace ppc::initializer;
using namespace ppc::storage;
using namespace ppc::front;
using namespace ppc::psi;
using namespace ppc::pir;
using namespace ppc::tools;
using namespace ppc::crypto;

Initializer::Initializer(std::string const& _configPath) : m_configPath(_configPath)
{
    // load the config
    m_config = std::make_shared<PPCConfig>();
    m_config->loadConfig(_configPath);
}

void Initializer::init(ppc::protocol::NodeArch _arch)
{
    INIT_LOG(INFO) << LOG_DESC("init the wedpr-component") << LOG_KV("arch", _arch);
    // load the protocol
    m_protocolInitializer = std::make_shared<ProtocolInitializer>();
    m_protocolInitializer->init(m_config);

    auto ppcMessageFactory = std::make_shared<PPCMessageFactory>();
    // init the frontService
    INIT_LOG(INFO) << LOG_DESC("init the frontService") << LOG_KV("agency", m_config->agencyID());
    auto frontThreadPool = std::make_shared<bcos::ThreadPool>("front", m_config->threadPoolSize());

    // Note: must set the  m_holdingMessageMinutes before init the node
    if (_arch == ppc::protocol::NodeArch::AIR)
    {
        m_frontInitializer = std::make_shared<FrontInitializer>(
            m_config->agencyID(), frontThreadPool, m_protocolInitializer->ppcMsgFactory());
        // load the gateway config
        m_config->loadGatewayConfig(ppc::protocol::NodeArch::AIR, nullptr, m_configPath);
    }
    else
    {
        m_frontInitializer = std::make_shared<ProFrontInitializer>(
            m_config->agencyID(), frontThreadPool, m_protocolInitializer->ppcMsgFactory());
        m_agencyInfoFetcher = std::make_shared<bcos::Timer>(3000, "agencyInfoFetcher");
        // Note: the timer start work only after calling start
        auto self = weak_from_this();
        m_agencyInfoFetcher->registerTimeoutHandler([self]() {
            auto init = self.lock();
            if (!init)
            {
                return;
            }
            init->fetchAgencyListPeriodically();
        });
        // load the tars config
        m_config->loadTarsConfig(m_configPath);
    }
    INIT_LOG(INFO) << LOG_DESC("init the frontService success")
                   << LOG_KV("agency", m_config->agencyID()) << LOG_KV("arch", _arch);

    auto cryptoBox = m_protocolInitializer->cryptoBox();
    SQLStorage::Ptr sqlStorage = nullptr;
    if (!m_config->disableRA2018())
    {
        // init the SQLStorage(without set the dbName)
        auto opt = std::make_shared<protocol::SQLConnectionOption>(
            *(m_config->storageConfig().sqlConnectionOpt));
        opt->database = "";
        INIT_LOG(INFO) << LOG_DESC("init the sqlStorage for ra2018-psi") << opt->desc();
        sqlStorage = m_protocolInitializer->sqlStorageFactory()->createSQLStorage(
            ppc::protocol::DataResourceType::MySQL, opt);
        INIT_LOG(INFO) << LOG_DESC("init the sqlStorage for ra2018-psi success");
    }
    // init the fileStorage for ra2018
    FileStorage::Ptr fileStorage = nullptr;
    if (!m_config->disableRA2018() && m_config->ra2018PSIConfig().useHDFS)
    {
        auto const& fileStorageOption = m_config->storageConfig().fileStorageConnectionOpt;
        INIT_LOG(INFO) << LOG_DESC("init the hdfs-storage for ra2018-psi")
                       << fileStorageOption->desc();
        fileStorage = m_protocolInitializer->fileStorageFactory()->createFileStorage(
            ppc::protocol::DataResourceType::HDFS, fileStorageOption);
        INIT_LOG(INFO) << LOG_DESC("init the hdfs-storage for ra2018-psi success");
    }

    // init the ra2018-psi
    INIT_LOG(INFO) << LOG_DESC("init the ra2018-psi");
    auto ra2018PSIFactory = std::make_shared<RA2018PSIFactory>();
    auto oprf = std::make_shared<RA2018Oprf>(
        m_config->privateKey(), cryptoBox->eccCrypto(), cryptoBox->hashImpl());
    m_ra2018PSI = ra2018PSIFactory->createRA2018PSI(m_config->agencyID(),
        m_frontInitializer->front(), m_config, oprf, m_protocolInitializer->binHashImpl(),
        m_protocolInitializer->ppcMsgFactory(), sqlStorage, fileStorage,
        std::make_shared<bcos::ThreadPool>("ra2018", m_config->threadPoolSize()),
        m_protocolInitializer->dataResourceLoader());
    INIT_LOG(INFO) << LOG_DESC("init the ra2018-psi success");

    // init the labeled-psi
    INIT_LOG(INFO) << LOG_DESC("init the labeled-psi");
    auto labeledPSIFactory = std::make_shared<LabeledPSIFactory>();
    m_labeledPSI = labeledPSIFactory->buildLabeledPSI(m_config->agencyID(),
        m_frontInitializer->front(), cryptoBox,
        std::make_shared<bcos::ThreadPool>("t_labeled-psi", m_config->threadPoolSize()),
        m_protocolInitializer->dataResourceLoader(), m_config->holdingMessageMinutes());
    INIT_LOG(INFO) << LOG_DESC("init the labeled-psi success");

    // init the cm2020-psi
    INIT_LOG(INFO) << LOG_DESC("init the cm2020-psi");
    auto cm2020PSIFactory = std::make_shared<CM2020PSIFactory>();
    m_cm2020PSI = cm2020PSIFactory->buildCM2020PSI(m_config->agencyID(),
        m_frontInitializer->front(), cryptoBox,
        std::make_shared<bcos::ThreadPool>("t_cm2020-psi", m_config->threadPoolSize()),
        m_protocolInitializer->dataResourceLoader(), m_config->holdingMessageMinutes(),
        m_config->cm2020PSIConfig().parallelism);
    INIT_LOG(INFO) << LOG_DESC("init the cm2020-psi success");

    // init the ecdh-psi
    INIT_LOG(INFO) << LOG_DESC("create ecdh-psi");
    auto ecdhPSIFactory = std::make_shared<EcdhPSIFactory>();
    auto ecdhCryptoFactory = std::make_shared<ECDHCryptoFactoryImpl>(m_config->privateKey());
    m_ecdhPSI = ecdhPSIFactory->createEcdhPSI(m_config, ecdhCryptoFactory,
        m_frontInitializer->front(), m_protocolInitializer->ppcMsgFactory(), nullptr,
        m_protocolInitializer->dataResourceLoader());
    INIT_LOG(INFO) << LOG_DESC("create ecdh-psi success");

#if 0
// TODO: optimize here
    // init the ecdh-conn-psi
    INIT_LOG(INFO) << LOG_DESC("create ecdh-conn-psi");
    auto ecdhConnPSIFactory = std::make_shared<EcdhConnPSIFactory>();
    m_ecdhConnPSI = ecdhConnPSIFactory->createEcdhConnPSI(m_config, ecdhCryptoFactory,
        m_frontInitializer->front(), m_protocolInitializer->ppcMsgFactory(),
        std::make_shared<bcos::ThreadPool>("t_ecdh-conn-psi", std::thread::hardware_concurrency()),
        m_protocolInitializer->dataResourceLoader());
    INIT_LOG(INFO) << LOG_DESC("create ecdh-conn-psi success");
#endif
    // init the ecdh-multi-psi
    INIT_LOG(INFO) << LOG_DESC("create ecdh-multi-psi");
    auto ecdhMultiPSIFactory = std::make_shared<EcdhMultiPSIFactory>();
    m_ecdhMultiPSI = ecdhMultiPSIFactory->createEcdhMultiPSI(m_config, m_frontInitializer->front(),
        cryptoBox,
        std::make_shared<bcos::ThreadPool>("t_ecdh-multi-psi", std::thread::hardware_concurrency()),
        m_protocolInitializer->dataResourceLoader());
    INIT_LOG(INFO) << LOG_DESC("create ecdh-multi-psi success");

    // init the ot-pir
    INIT_LOG(INFO) << LOG_DESC("create ot-pir");
    auto otPIRFactory = std::make_shared<OtPIRFactory>();
    m_otPIR = otPIRFactory->buildOtPIR(m_config->agencyID(), m_frontInitializer->front(), cryptoBox,
        std::make_shared<bcos::ThreadPool>("t_ot-pir", std::thread::hardware_concurrency()),
        m_protocolInitializer->dataResourceLoader(), m_config->holdingMessageMinutes());

    // auto otPIRImpl = std::make_shared<OtPIRImpl>();
    // m_otPIR = otPIRImpl;
    // init the bs mode ecdh psi
    INIT_LOG(INFO) << LOG_DESC("create bs mode ecdh psi");
    auto bsEcdhPSIFactory = std::make_shared<BsEcdhPSIFactory>();
    m_bsEcdhPSI = bsEcdhPSIFactory->buildBsEcdhPSI(
        std::make_shared<bcos::ThreadPool>("t_bs-ecdh-psi", m_config->threadPoolSize()),
        m_protocolInitializer->dataResourceLoader(), m_config->taskTimeoutMinutes());
    INIT_LOG(INFO) << LOG_DESC("create bs mode ecdh psi success");

    initMsgHandlers();

    // add parallelism control for tbb
    tbb::global_control gc(
        tbb::global_control::max_allowed_parallelism, m_config->threadPoolSize());
}

// init the msg-handlers
void Initializer::initMsgHandlers()
{
    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for ra2018PSI");
    // register msg-handlers for ra2018-psi
    auto weakRA2018PSI = std::weak_ptr<RA2018PSIImpl>(m_ra2018PSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::RA_PSI_2PC,
        [weakRA2018PSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakRA2018PSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });

    // register msg-handlers for labeled-psi
    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for labeledPSI");
    auto weakLabeledPSI = std::weak_ptr<LabeledPSIImpl>(m_labeledPSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::LABELED_PSI_2PC,
        [weakLabeledPSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakLabeledPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });

    // register msg-handlers for cm2020-psi
    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for CM2020PSI");
    auto weakCM2020PSI = std::weak_ptr<CM2020PSIImpl>(m_cm2020PSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::CM_PSI_2PC,
        [weakCM2020PSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakCM2020PSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });

    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for ecdh-psi");
    // register msg-handlers for ecdh-psi
    auto weakEcdhPSI = std::weak_ptr<EcdhPSIImpl>(m_ecdhPSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_2PC,
        [weakEcdhPSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakEcdhPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });

    // register msg-handlers for ecdh-conn-psi
    /*INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for ecdh-conn-psi");
    auto weakEcdhConnPSI = std::weak_ptr<EcdhConnPSIImpl>(m_ecdhConnPSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_CONN,
        [weakEcdhConnPSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakEcdhConnPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });*/

    // register msg-handlers for ecdh-multi-psi
    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for ecdh-multi-psi");
    auto weakEcdhMultiPSI = std::weak_ptr<EcdhMultiPSIImpl>(m_ecdhMultiPSI);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_MULTI,
        [weakEcdhMultiPSI](ppc::front::PPCMessageFace::Ptr _msg) {
            auto psi = weakEcdhMultiPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->onReceiveMessage(_msg);
        });

    INIT_LOG(INFO) << LOG_DESC("initMsgHandlers for ot-pir");
    // register msg-handlers for ecdh-psi
    auto weakOtPIR = std::weak_ptr<OtPIRImpl>(m_otPIR);
    m_frontInitializer->front()->registerMessageHandler((uint8_t)ppc::protocol::TaskType::PIR,
        (uint8_t)ppc::protocol::PSIAlgorithmType::OT_PIR_2PC,
        [weakOtPIR](ppc::front::PPCMessageFace::Ptr _msg) {
            auto pir = weakOtPIR.lock();
            if (!pir)
            {
                return;
            }
            pir->onReceiveMessage(_msg);
        });
}

void Initializer::registerRpcHandler(ppc::rpc::RpcInterface::Ptr const& _rpc)
{
    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler");

    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for ra2018PSI");
    // register task handler for ra2018-psi
    auto weakRA2018PSI = std::weak_ptr<RA2018PSIImpl>(m_ra2018PSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::RA_PSI_2PC,
        [weakRA2018PSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto ra2018Impl = weakRA2018PSI.lock();
            if (!ra2018Impl)
            {
                return;
            }
            ra2018Impl->asyncRunTask(_task, std::move(_handler));
        });

    // register task handler for labeled-psi
    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for labeledPSI");
    auto weakLabeledPSI = std::weak_ptr<LabeledPSIImpl>(m_labeledPSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::LABELED_PSI_2PC,
        [weakLabeledPSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto labeledPSI = weakLabeledPSI.lock();
            if (!labeledPSI)
            {
                return;
            }
            labeledPSI->asyncRunTask(_task, std::move(_handler));
        });

    // register task handler for cm2020-psi
    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for cm2020PSI");
    auto weakCM2020PSI = std::weak_ptr<CM2020PSIImpl>(m_cm2020PSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::CM_PSI_2PC,
        [weakCM2020PSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto cm2020PSI = weakCM2020PSI.lock();
            if (!cm2020PSI)
            {
                return;
            }
            cm2020PSI->asyncRunTask(_task, std::move(_handler));
        });

    // register task handler for ecdh-psi
    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for ecdhPSI");
    auto weakEcdhPSI = std::weak_ptr<EcdhPSIImpl>(m_ecdhPSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_2PC,
        [weakEcdhPSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto psi = weakEcdhPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->asyncRunTask(_task, std::move(_handler));
        });

    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for ecdhMultiPSI");
    auto weakEcdhMultiPSI = std::weak_ptr<EcdhMultiPSIImpl>(m_ecdhMultiPSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_MULTI,
        [weakEcdhMultiPSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto psi = weakEcdhMultiPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->asyncRunTask(_task, std::move(_handler));
        });

    /*INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for ecdhConnPSI");
    auto weakEcdhConnPSI = std::weak_ptr<EcdhConnPSIImpl>(m_ecdhConnPSI);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PSI,
        (uint8_t)ppc::protocol::PSIAlgorithmType::ECDH_PSI_CONN,
        [weakEcdhConnPSI](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr&&)> _handler) {
            auto psi = weakEcdhConnPSI.lock();
            if (!psi)
            {
                return;
            }
            psi->asyncRunTask(_task, std::move(_handler));
        });*/

    // register task handler for ot-pir
    INIT_LOG(INFO) << LOG_DESC("registerRpcHandler for otPIR");
    auto weakOtPIR = std::weak_ptr<OtPIRImpl>(m_otPIR);
    _rpc->registerTaskHandler(ppc::protocol::TaskType::PIR,
        (uint8_t)ppc::protocol::PSIAlgorithmType::OT_PIR_2PC,
        [weakOtPIR](ppc::protocol::Task::ConstPtr _task,
            std::function<void(ppc::protocol::TaskResult::Ptr &&)> _handler) {
            auto pir = weakOtPIR.lock();
            if (!pir)
            {
                return;
            }
            pir->asyncRunTask(_task, std::move(_handler));
        });
}

void Initializer::start()
{
    if (m_agencyInfoFetcher)
    {
        m_agencyInfoFetcher->start();
    }
    if (m_frontInitializer)
    {
        m_frontInitializer->start();
    }
    /*if (m_ecdhConnPSI)
    {
        m_ecdhConnPSI->start();
    }*/
    if (m_ecdhMultiPSI)
    {
        m_ecdhMultiPSI->start();
    }
    if (m_cm2020PSI)
    {
        m_cm2020PSI->start();
    }
    if (m_ra2018PSI)
    {
        m_ra2018PSI->start();
    }
    if (m_labeledPSI)
    {
        m_labeledPSI->start();
    }
    // start the ecdh-psi
    if (m_ecdhPSI)
    {
        m_ecdhPSI->start();
    }
    if (m_otPIR)
    {
        m_otPIR->start();
    }

    if (m_bsEcdhPSI)
    {
        m_bsEcdhPSI->start();
    }
}

void Initializer::stop()
{
    if (m_agencyInfoFetcher)
    {
        m_agencyInfoFetcher->stop();
    }
    // stop the network firstly
    if (m_frontInitializer)
    {
        m_frontInitializer->stop();
    }
    /*if (m_ecdhConnPSI)
    {
        m_ecdhConnPSI->stop();
    }*/
    if (m_ecdhMultiPSI)
    {
        m_ecdhMultiPSI->stop();
    }
    if (m_cm2020PSI)
    {
        m_cm2020PSI->stop();
    }
    // if (m_ra2018PSI)
    // {
    //     m_ra2018PSI->stop();
    // }
    // if (m_labeledPSI)
    // {
    //     m_labeledPSI->stop();
    // }
    // stop the ecdh-psi
    if (m_ecdhPSI)
    {
        m_ecdhPSI->stop();
    }
    if (m_otPIR)
    {
        m_otPIR->stop();
    }
    if (m_bsEcdhPSI)
    {
        m_bsEcdhPSI->stop();
    }
}

void Initializer::fetchAgencyListPeriodically()
{
    if (!m_agencyInfoFetcher)
    {
        return;
    }
    fetchAgencyList();
    m_agencyInfoFetcher->restart();
}

// fetch the agency-list for ecdh and ra2018 periodically in pro-mode
void Initializer::fetchAgencyList()
{
    auto weakEcdhPSI = std::weak_ptr<EcdhPSIImpl>(m_ecdhPSI);
    auto weakRA2018PSI = std::weak_ptr<RA2018PSIImpl>(m_ra2018PSI);
    m_frontInitializer->front()->asyncGetAgencyList(
        [weakEcdhPSI, weakRA2018PSI](
            bcos::Error::Ptr _error, std::vector<std::string>&& _agencyList) {
            if (_error)
            {
                INIT_LOG(INFO) << LOG_DESC("asyncGetAgencyList failed")
                               << LOG_KV("code", _error->errorCode());
                return;
            }
            auto ecdhPsi = weakEcdhPSI.lock();
            if (ecdhPsi)
            {
                ecdhPsi->psiConfig()->updateAgenyList(_agencyList);
            }
            auto ra2018Psi = weakRA2018PSI.lock();
            if (ra2018Psi)
            {
                ra2018Psi->psiConfig()->updateAgenyList(_agencyList);
            }
        });
}