#pragma once
#include "EcdhMultiPSIMessageFactory.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-psi/src/PSIConfig.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/ThreadPool.h>
#include <gperftools/malloc_extension.h>
#include <utility>

namespace ppc::psi
{
class EcdhMultiPSIConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIConfig>;
    EcdhMultiPSIConfig(std::string const& _selfPartyID, ppc::front::FrontInterface::Ptr _front,
        ppc::crypto::CryptoBox::Ptr _cryptoBox, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, uint32_t _dataBatchSize,
        int _holdingMessageMinutes, EcdhMultiPSIMessageFactory::Ptr const& _psiMsgFactory,
        const front::PPCMessageFactory::Ptr& _ppcMsgFactory =
            std::make_shared<front::PPCMessageFactory>())
      : PSIConfig(ppc::protocol::TaskAlgorithmType::ECDH_PSI_MULTI, _selfPartyID, std::move(_front),
            _ppcMsgFactory, std::move(_dataResourceLoader), _holdingMessageMinutes),
        m_threadPool(std::move(_threadPool)),
        m_cryptoBox(std::move(_cryptoBox)),
        m_psiMsgFactory(std::move(_psiMsgFactory)),
        m_dataBatchSize(_dataBatchSize)
    {}

    virtual ~EcdhMultiPSIConfig()
    {
        if (m_threadPool)
        {
            m_threadPool->stop();
        }
    };

    EcdhMultiPSIMessageFactory::Ptr const& psiMsgFactory() const { return m_psiMsgFactory; }
    crypto::Hash::Ptr const& hash() const { return m_cryptoBox->hashImpl(); }
    crypto::EccCrypto::Ptr const& eccCrypto() const { return m_cryptoBox->eccCrypto(); }
    crypto::CryptoBox::Ptr const& cryptoBox() const { return m_cryptoBox; }
    bcos::ThreadPool::Ptr const& threadPool() const { return m_threadPool; }
    uint32_t dataBatchSize() const { return m_dataBatchSize; }

private:
    ppc::crypto::CryptoBox::Ptr m_cryptoBox;
    bcos::ThreadPool::Ptr m_threadPool;
    EcdhMultiPSIMessageFactory::Ptr m_psiMsgFactory;
    uint32_t m_dataBatchSize = 10000;
};
}  // namespace ppc::psi