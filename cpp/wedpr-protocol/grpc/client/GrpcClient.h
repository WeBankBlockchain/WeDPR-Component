/**
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
 * @file GrpcClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "Service.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace ppc::protocol
{
// struct for keeping state and data information
class AsyncClientCall
{
public:
    using CallbackDef =
        std::function<void(grpc::ClientContext const&, grpc::Status const&, ppc::proto::Error&&)>;
    AsyncClientCall(CallbackDef _callback) : callback(std::move(_callback)) {}

    CallbackDef callback;
    // Container for the data we expect from the server.
    ppc::proto::Error reply;
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    // Storage for the status of the RPC upon completion.
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<ppc::proto::Error>> responseReader;
};

class GrpcClient
{
public:
    using Ptr = std::shared_ptr<GrpcClient>;
    GrpcClient(std::shared_ptr<grpc::Channel> channel) : m_channel(std::move(channel)) {}

    virtual ~GrpcClient() = default;

    std::shared_ptr<grpc::Channel> const& channel() { return m_channel; }
    grpc::CompletionQueue& queue() { return m_queue; }

    void handleRpcResponse();

private:
    std::shared_ptr<grpc::Channel> m_channel;
    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    // TODO: check threadsafe
    grpc::CompletionQueue m_queue;
};
}  // namespace ppc::protocol