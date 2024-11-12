/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file PSIConnMessageFactory.cpp
 * @author: zachma
 * @date 2023-8-23
 */

#pragma once
#include "PSIConnMessageFactory.h"
#include "ecc.pb.h"
#include "ecdh_psi.pb.h"
#include "entry.pb.h"
#include "header.pb.h"
#include "psi.pb.h"
#include "transport.pb.h"
#include <google/protobuf/any.pb.h>

using namespace ppc::psi;

bcos::bytes PSIConnMessageFactory::createHandshakeRequest(
    HandShakeRequestVo::Ptr handShakeRequestVo)
{
    org::interconnection::v2::HandshakeRequest handShakeRequest;
    handShakeRequest.set_version(2);
    handShakeRequest.set_requester_rank((int32_t)EcdhConnProcess::HandShakeProcess);
    handShakeRequest.add_supported_algos(org::interconnection::v2::AlgoType::ALGO_TYPE_ECDH_PSI);
    handShakeRequest.add_protocol_families(
        org::interconnection::v2::ProtocolFamily::PROTOCOL_FAMILY_ECC);

    // create eccProtocol
    org::interconnection::v2::protocol::EccProtocolProposal eccProtocol;
    eccProtocol.add_supported_versions(2);
    eccProtocol.add_point_octet_formats(
        org::interconnection::v2::protocol::PointOctetFormat::POINT_OCTET_FORMAT_X962_COMPRESSED);
    eccProtocol.set_support_point_truncation(false);

    // create eccSuit
    org::interconnection::v2::protocol::EcSuit* eccSuit = eccProtocol.add_ec_suits();
    auto _curve = handShakeRequestVo->GetCurve().cbegin();
    auto _hash = handShakeRequestVo->GetHash().cbegin();

#ifdef ENABLE_CONN
    eccSuit->set_curve(2);  // SM2
    eccSuit->set_hash(11);  // SHA256
#else
    eccSuit->set_curve(*_curve);
    eccSuit->set_hash(*_hash);
#endif

    eccSuit->set_hash2curve_strategy(org::interconnection::v2::protocol::HashToCurveStrategy::
            HASH_TO_CURVE_STRATEGY_TRY_AND_REHASH);

    // google::protobuf::Any protocol_families_params_any;
    handShakeRequest.add_protocol_family_params()->PackFrom(eccProtocol);

    // create PsiDataIoProposal
    org::interconnection::v2::algos::PsiDataIoProposal psiDataIo;
    psiDataIo.add_supported_versions(2);
    psiDataIo.set_result_to_rank(-1);
    psiDataIo.set_item_num(handShakeRequestVo->GetItemCount());

    handShakeRequest.mutable_io_param()->PackFrom(psiDataIo);
    bcos::bytes result;
    result.resize(handShakeRequest.ByteSize());
    handShakeRequest.SerializeToArray(result.data(), handShakeRequest.ByteSize());
    return result;
}

HandShakeRequestVo::Ptr PSIConnMessageFactory::parseHandshakeRequest(const bcos::bytes& _value)
{
    std::string msgStr = "";
    msgStr.assign(_value.begin(), _value.end());

    // parse HandshakeRequest
    org::interconnection::v2::HandshakeRequest handShakeRequest;
    handShakeRequest.ParseFromString(msgStr);

    auto supported_algos = handShakeRequest.supported_algos(0);
    auto protocol_families = handShakeRequest.protocol_families(0);
    auto protocol_family_params = handShakeRequest.protocol_family_params(0);
    std::set<int32_t> curves;
    std::set<int32_t> hashes;
    int32_t item_num = -1;
    if (protocol_family_params.Is<org::interconnection::v2::protocol::EccProtocolProposal>())
    {
        org::interconnection::v2::protocol::EccProtocolProposal eccProtocol;
        protocol_family_params.UnpackTo(&eccProtocol);
        auto ecc_suits = eccProtocol.ec_suits();
        for (auto const& suit : ecc_suits)
        {
            curves.insert(suit.curve());
            hashes.insert(suit.hash());
        }
    }

    auto io_param = handShakeRequest.io_param();
    if (io_param.Is<org::interconnection::v2::algos::PsiDataIoProposal>())
    {
        org::interconnection::v2::algos::PsiDataIoProposal psiDataIo;
        io_param.UnpackTo(&psiDataIo);
        item_num = psiDataIo.item_num();
    }

    return std::make_shared<HandShakeRequestVo>(curves, hashes, protocol_families, item_num);
}

bcos::bytes PSIConnMessageFactory::createHandshakeResponse(
    HandShakeResponseVo::Ptr _handShakeResponseVo)
{
    std::string result;

    // create response header
    org::interconnection::ResponseHeader* responseHeader =
        new org::interconnection::ResponseHeader();
    responseHeader->set_error_code(_handShakeResponseVo->GetErrorCode());
    responseHeader->set_allocated_error_msg(new std::string(_handShakeResponseVo->GetErrorMsg()));

    // create handShakeResponse
    org::interconnection::v2::HandshakeResponse handShakeResponse;
    handShakeResponse.set_allocated_header(responseHeader);
    handShakeResponse.set_algo(org::interconnection::v2::AlgoType::ALGO_TYPE_ECDH_PSI);
    handShakeResponse.add_protocol_families(
        org::interconnection::v2::ProtocolFamily::PROTOCOL_FAMILY_ECC);

    // create eccSuit
    org::interconnection::v2::protocol::EcSuit* eccSuit =
        new org::interconnection::v2::protocol::EcSuit();
    eccSuit->set_curve(_handShakeResponseVo->GetCurve());
    eccSuit->set_hash(_handShakeResponseVo->GetHash());
    eccSuit->set_hash2curve_strategy(3);

    // create EccProtocolResult
    org::interconnection::v2::protocol::EccProtocolResult eccProtocol;
    eccProtocol.set_version(1);
    eccProtocol.set_point_octet_format(1);
    eccProtocol.set_bit_length_after_truncated(-1);
    eccProtocol.set_allocated_ec_suit(eccSuit);

    google::protobuf::Any protocol_families_params_any;
    protocol_families_params_any.PackFrom(eccProtocol);
    handShakeResponse.add_protocol_family_params()->CopyFrom(protocol_families_params_any);

    // create PsiDataIoResult
    org::interconnection::v2::algos::PsiDataIoResult psiDataIo;
    psiDataIo.set_version(1);
    psiDataIo.set_result_to_rank(-1);


    google::protobuf::Any* io_param_any = new google::protobuf::Any();
    io_param_any->PackFrom(psiDataIo);
    handShakeResponse.set_allocated_io_param(io_param_any);

    handShakeResponse.SerializeToString(&result);
    return bcos::bytes(result.begin(), result.end());
}

HandShakeResponseVo::Ptr PSIConnMessageFactory::parseHandshakeResponse(const bcos::bytes& _value)
{
    std::string msgStr = "";
    msgStr.assign(_value.begin(), _value.end());

    org::interconnection::v2::HandshakeResponse handShakeResponse;
    handShakeResponse.ParseFromString(msgStr);

    auto responseHeader = handShakeResponse.header();

    auto errCode = responseHeader.error_code();
    auto errMsg = responseHeader.error_msg();
    auto algo = handShakeResponse.algo();
    auto protocol_families = handShakeResponse.protocol_families(0);
    auto protocol_family_params = handShakeResponse.protocol_family_params(0);
    int32_t curve;
    int32_t hashtype;
    if (protocol_family_params.Is<org::interconnection::v2::protocol::EccProtocolResult>())
    {
        org::interconnection::v2::protocol::EccProtocolResult eccProtocol;
        protocol_family_params.UnpackTo(&eccProtocol);
        auto ecc_suit = eccProtocol.ec_suit();
        curve = ecc_suit.curve();
        hashtype = ecc_suit.hash();
    }
    auto handRespVo = std::make_shared<HandShakeResponseVo>();
    handRespVo->SetCurve(curve);
    handRespVo->SetHash(hashtype);
    handRespVo->SetErrorCode(errCode);
    handRespVo->SetErrorMessage(errMsg);
    handRespVo->SetProtocolFamilies(protocol_families);

    return handRespVo;
}

bcos::bytes PSIConnMessageFactory::createCipherExchange(CipherBatchVo::Ptr _cipherBatchVo)
{
    org::interconnection::v2::runtime::EcdhPsiCipherBatch ecdhPsiCipherBatch;
    ecdhPsiCipherBatch.set_allocated_type(new std::string(_cipherBatchVo->type()));
    ecdhPsiCipherBatch.set_is_last_batch(_cipherBatchVo->is_last_batch());
    ecdhPsiCipherBatch.set_batch_index(_cipherBatchVo->batch_index());
    ecdhPsiCipherBatch.set_count(_cipherBatchVo->count());
    ecdhPsiCipherBatch.set_allocated_ciphertext(
        new std::string(encodeVectorBytesToString(_cipherBatchVo->cipherText())));

    std::string result;
    ecdhPsiCipherBatch.SerializeToString(&result);

    return bcos::bytes(result.begin(), result.end());
}

CipherBatchVo::Ptr PSIConnMessageFactory::parseCipherExchange(const bcos::bytes& _value)
{
    std::string msgStr = "";
    msgStr.assign(_value.begin(), _value.end());

    org::interconnection::v2::runtime::EcdhPsiCipherBatch ecdhPsiCipherBatch;
    ecdhPsiCipherBatch.ParseFromString(msgStr);

    auto cipherBatchVo = std::make_shared<CipherBatchVo>();
    cipherBatchVo->setType(ecdhPsiCipherBatch.type());
    cipherBatchVo->setBatchIndex(ecdhPsiCipherBatch.batch_index());
    cipherBatchVo->setIsLastBatch(ecdhPsiCipherBatch.is_last_batch());
    cipherBatchVo->setCount(ecdhPsiCipherBatch.count());
    cipherBatchVo->setCipherText(
        decodeStringToVectorBytes(ecdhPsiCipherBatch.ciphertext(), ecdhPsiCipherBatch.count()));
    return cipherBatchVo;
}

bcos::bytes PSIConnMessageFactory::createPSIConnMessageRequest(
    const bcos::bytes& _value, const std::string& _key)
{
    org::interconnection::link::PushRequest pushRequest;
    pushRequest.set_sender_rank(1);
    pushRequest.set_key(_key);
    pushRequest.set_trans_type(org::interconnection::link::TransType::MONO);
    pushRequest.set_value(_value.data(), _value.size());

    bcos::bytes result;
    result.resize(pushRequest.ByteSize());
    pushRequest.SerializeToArray(result.data(), pushRequest.ByteSize());

    return result;
}