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
 * @file Initializer.h
 * @author: yujiechen
 * @date 2022-11-14
 */
#pragma once
#include "ProtocolInitializer.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/gateway/IGateway.h"
#include "ppc-framework/rpc/RpcInterface.h"
#include "ppc-framework/rpc/RpcTypeDef.h"
#include "ppc-psi/src/bs-ecdh-psi/BsEcdhPSIImpl.h"
#include "wedpr-protocol/protocol/src/ServiceConfig.h"
#include "wedpr-transport/sdk/src/Transport.h"
#include <bcos-boostssl/httpserver/Common.h>
#include <bcos-utilities/Timer.h>


namespace ppc::psi
{
class RA2018PSIImpl;
class LabeledPSIImpl;
class CM2020PSIImpl;
class EcdhPSIImpl;
class EcdhMultiPSIImpl;
// class EcdhConnPSIImpl;
}  // namespace ppc::psi

namespace ppc::pir
{
class OtPIRImpl;
}  // namespace ppc::pir

namespace ppc::sdk
{
class TransportBuilder;
}

namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::initializer
{
class Initializer : public std::enable_shared_from_this<Initializer>
{
public:
    using Ptr = std::shared_ptr<Initializer>;
    Initializer(ppc::protocol::NodeArch _arch, std::string const& _configPath);
    virtual ~Initializer() { stop(); }

    // init the service
    virtual void init(ppc::gateway::IGateway::Ptr const& gateway);
    virtual void stop();
    virtual void start();

    std::shared_ptr<ppc::tools::PPCConfig> config() { return m_config; }
    ppc::sdk::Transport::Ptr const& transport() const { return m_transport; }
    ppc::front::FrontInterface::Ptr const& ppcFront() const { return m_ppcFront; }

    std::shared_ptr<ppc::tools::PPCConfig> const& config() const { return m_config; }
    ProtocolInitializer::Ptr const& protocolInitializer() const { return m_protocolInitializer; }
    ppc::psi::BsEcdhPSIImpl::Ptr const& bsEcdhPsi() const { return m_bsEcdhPSI; }

    void registerRpcHandler(ppc::rpc::RpcInterface::Ptr const& _rpc);

protected:
    virtual void initMsgHandlers();


private:
    uint16_t m_arch;
    std::string m_configPath;
    std::shared_ptr<ppc::tools::PPCConfig> m_config;
    ProtocolInitializer::Ptr m_protocolInitializer;
    ppc::protocol::ServiceConfigBuilder m_serviceConfigBuilder;

    std::shared_ptr<ppc::sdk::TransportBuilder> m_transportBuilder;
    ppc::sdk::Transport::Ptr m_transport;
    // created using transport
    ppc::front::FrontInterface::Ptr m_ppcFront;

    // the ra2018-psi
    std::shared_ptr<ppc::psi::RA2018PSIImpl> m_ra2018PSI;
    // the labeled-psi
    std::shared_ptr<ppc::psi::LabeledPSIImpl> m_labeledPSI;
    // the cm2020-psi
    std::shared_ptr<ppc::psi::CM2020PSIImpl> m_cm2020PSI;
    // the ecdh-psi
    std::shared_ptr<ppc::psi::EcdhPSIImpl> m_ecdhPSI;
    // the ecdh-multi-psi
    std::shared_ptr<ppc::psi::EcdhMultiPSIImpl> m_ecdhMultiPSI;
    // the ecdh-conn-psi
    // std::shared_ptr<ppc::psi::EcdhConnPSIImpl> m_ecdhConnPSI;

    std::shared_ptr<ppc::pir::OtPIRImpl> m_otPIR;

    std::shared_ptr<ppc::psi::BsEcdhPSIImpl> m_bsEcdhPSI;
};
}  // namespace ppc::initializer