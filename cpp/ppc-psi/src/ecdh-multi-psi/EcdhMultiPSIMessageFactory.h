#pragma once
#include "Common.h"
#include "ppc-psi/src/psi-framework/protocol/PSIMessage.h"
namespace ppc::psi
{
class EcdhMultiPSIMessageFactory : public PSIMessageFactoryImpl
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIMessageFactory>;
    EcdhMultiPSIMessageFactory() = default;
    ~EcdhMultiPSIMessageFactory() override = default;

    PSIMessageInterface::Ptr decodePSIMessage(bcos::bytesConstRef _data) override
    {
        auto inner = [inner = ppctars::PSIMessage()]() mutable { return &inner; };
        tars::TarsInputStream<tars::BufferReader> input;
        input.setBuffer((const char*)_data.data(), _data.size());
        inner()->readFrom(input);
        switch (inner()->packetType)
        {
        case (uint32_t)EcdhMultiPSIMessageType::GENERATE_RANDOM_TO_PARTNER:
        case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_CALCULATOR:
        case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_PARTNER:
        case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_CALCULATOR:
        case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR:
        case (uint32_t)
            EcdhMultiPSIMessageType::RETURN_ENCRYPTED_INTERSECTION_SET_FROM_CALCULATOR_TO_MASTER:
        case (uint32_t)EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL:
            return std::make_shared<PSIMessage>(inner);
        default:
            return decodePSIBaseMessage(inner);
        }
    }
};
}  // namespace ppc::psi
