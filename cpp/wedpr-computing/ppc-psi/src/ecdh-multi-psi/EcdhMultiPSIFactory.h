#pragma once
#include "EcdhMultiPSIImpl.h"
#include "EcdhMultiPSIMessageFactory.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-tools/src/config/PPCConfig.h"

namespace ppc::psi
{
class EcdhMultiPSIFactory
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIFactory>;
    EcdhMultiPSIFactory() = default;
    virtual ~EcdhMultiPSIFactory() = default;

    virtual EcdhMultiPSIImpl::Ptr createEcdhMultiPSI(ppc::tools::PPCConfig::Ptr const& _ppcConfig,
        ppc::front::FrontInterface::Ptr _front, ppc::crypto::CryptoBox::Ptr _cryptoBox,
        bcos::ThreadPool::Ptr _threadPool, ppc::io::DataResourceLoader::Ptr _dataResourceLoader)
    {
        auto psiMsgFactory = std::make_shared<EcdhMultiPSIMessageFactory>();
        auto const& ecdhParam = _ppcConfig->ecdhMultiPSIConfig();
        auto _selfParty = _ppcConfig->agencyID();
        int _holdingMessageMinutes = _ppcConfig->holdingMessageMinutes();
        auto config = std::make_shared<EcdhMultiPSIConfig>(_selfParty, std::move(_front),
            std::move(_cryptoBox), std::move(_threadPool), std::move(_dataResourceLoader),
            ecdhParam.dataBatchSize, _holdingMessageMinutes, psiMsgFactory);
        return std::make_shared<EcdhMultiPSIImpl>(config);
    }
};
}  // namespace ppc::psi