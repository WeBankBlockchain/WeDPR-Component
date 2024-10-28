%define MODULEIMPORT
"
# Import the low-level C/C++ module
from wedpr_python_gateway_sdk.libs import _wedpr_python_transport
"
%enddef
 
%module(moduleimport=MODULEIMPORT) wedpr_python_transport
%module(directors="1") wedpr_python_transport

%include <stdint.i>
%include <cpointer.i>
%include <std_vector.i>
%include <std_string.i>
%include <std_shared_ptr.i>
%include "typemaps.i"

// shared_ptr definition
%shared_ptr(ppc::front::FrontConfig);
%shared_ptr(ppc::front::IFront);
%shared_ptr(ppc::front::IFrontClient);
// the callbacks
%shared_ptr(ppc::front::ErrorCallback);
%shared_ptr(ppc::front::MessageDispatcherHandler);
%shared_ptr(ppc::front::IMessageHandler);
%shared_ptr(ppc::front::GetPeersInfoHandler);

%shared_ptr(ppc::gateway::IGateway);
%shared_ptr(bcos::Error);
%shared_ptr(bcos::bytes);
%shared_ptr(ppc::protocol::Message);
%shared_ptr(ppc::protocol::MessageOptionalHeader);
%shared_ptr(ppc::protocol::MessageHeader);
%shared_ptr(ppc::protocol::MessagePayload);
%shared_ptr(ppc::protocol::MessageBuilder);
%shared_ptr(ppc::protocol::MessageHeaderBuilder);
%shared_ptr(ppc::protocol::MessagePayloadBuilder);
%shared_ptr(ppc::protocol::MessageOptionalHeaderBuilder);
%shared_ptr(ppc::protocol::GrpcConfig);
%shared_ptr(ppc::sdk::Transport);

%{
#define SWIG_FILE_WITH_INIT
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <stdint.h>
#include "wedpr-transport/sdk/src/TransportBuilder.h"
#include "wedpr-transport/sdk/src/Transport.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/protocol/RouteType.h"
#include "ppc-framework/front/FrontConfig.h"
#include "ppc-framework/protocol/GrpcConfig.h"
#include <bcos-utilities/Error.h>
#include "ppc-framework/protocol/EndPoint.h"
#include "ppc-framework/protocol/Message.h"
#include "ppc-framework/protocol/MessagePayload.h"
%}
namespace ppc::sdk{
    class Transport;
    class TransportBuilder;
}

namespace ppc::gateway{
    class IGateway;
}

namespace ppc::protocol{
    class Message;
    class MessageHeader;
    class MessagePayload;
    class MessageOptionalHeader;
    class MessageBuilder;
    class MessageHeaderBuilder;
    class MessagePayloadBuilder;
    class MessageOptionalHeaderBuilder;
    class EndPoint;
    class GrpcConfig;
    class RouteType;
}

namespace ppc::front{
    class FrontConfig;
    class IFront;
    class IFrontClient;
    class FrontImpl;
    class FrontBuilderImpl;
    class GatewayEndPoint;
    class ErrorCallback;
    class MessageDispatcherHandler;
    class IMessageHandler;
    class SendResponseHandler;
}

namespace std{
    class vector;
    class string;
}

namespace bcos{
    using byte = uint8_t;
    using bytes = std::vector<byte>;
    class Error;
}

// define shared_ptr objects
%template(SharedBcosError) std::shared_ptr<bcos::Error>;

%template(SharedFrontConfig) std::shared_ptr<ppc::front::FrontConfig>;
%template(SharedGrpcConfig) std::shared_ptr<ppc::protocol::GrpcConfig>;

%template(SharedFront) std::shared_ptr<ppc::front::IFront>;
%template(SharedFrontClient) std::shared_ptr<ppc::front::IFrontClient>;

%template(SharedErrorCallback) std::shared_ptr<ppc::front::ErrorCallback>;
%template(SharedMessageDispatcherHandler) std::shared_ptr<ppc::front::MessageDispatcherHandler>;
%template(SharedIMessageHandler) std::shared_ptr<ppc::front::IMessageHandler>;
%template(SharedGetPeersInfoHandler) std::shared_ptr<ppc::front::GetPeersInfoHandler>;

%template(SharedGateway) std::shared_ptr<ppc::gateway::IGateway>;

%template(SharedMessage) std::shared_ptr<ppc::protocol::Message>;
%template(SharedMessageHeader) std::shared_ptr<ppc::protocol::MessageHeader>;
%template(SharedMessagePayload) std::shared_ptr<ppc::protocol::MessagePayload>;
%template(SharedRouteInfo) std::shared_ptr<ppc::protocol::MessageOptionalHeader>;

%template(SharedMessageBuilder) std::shared_ptr<ppc::protocol::MessageBuilder>;
%template(SharedMessageHeaderBuilder) std::shared_ptr<ppc::protocol::MessageHeaderBuilder>;
%template(SharedMessagePayloadBuilder) std::shared_ptr<ppc::protocol::MessagePayloadBuilder>;
%template(SharedRouteInfoBuilder) std::shared_ptr<ppc::protocol::MessageOptionalHeaderBuilder>;

%template(ubytes) std::vector<uint8_t>;
%template(ibytes) std::vector<int8_t>;
%template(StringVec) std::vector<std::string>;

%include <pybuffer.i>
%pybuffer_binary(char* data, uint64_t length)
%pybuffer_binary(char* payload, uint64_t payloadSize)

%typemap(out) OutputBuffer {
    $result = PyBytes_FromStringAndSize((const char *)$1.data, $1.len);
}

/// callbacks
%feature("director") ppc::front::ErrorCallback;
%feature("director") ppc::front::MessageDispatcherHandler;
%feature("director") ppc::front::IMessageHandler;
%feature("director") ppc::front::GetPeersInfoHandler;

// the method no need to wrapper
%ignore ppc::sdk::TransportBuilder::build;
%ignore ppc::front::IFront::onReceiveMessage;
%ignore ppc::front::IFront::asyncSendMessage;
%ignore ppc::front::IFront::asyncGetAgencies;
%ignore ppc::front::IFront::registerTopicHandler;
%ignore ppc::front::IFront::registerMessageHandler;
%ignore ppc::front::IFront::asyncSendResponse;
%ignore ppc::front::IFront::populateErrorCallback;
%ignore ppc::front::IFront::populateMessageDispatcherCallback;
%ignore ppc::front::IFront::populateMsgCallback;
%ignore ppc::front::IFront::push;
%ignore ppc::front::IFront::registerNodeInfo;
%ignore ppc::front::IFront::unRegisterNodeInfo;
%ignore ppc::protocol::MessageOptionalHeader::srcNode;
%ignore ppc::protocol::MessageOptionalHeader::dstNode;
%ignore ppc::protocol::MessageOptionalHeader::setDstNode;
%ignore ppc::protocol::MessageOptionalHeader::setDstNodePtr;
%ignore ppc::protocol::MessageOptionalHeader::setSrcNode;
%ignore ppc::protocol::MessagePayload::data;
%ignore ppc::protocol::MessagePayload::setData;
%ignore ppc::protocol::MessagePayload::setDataPtr;
%ignore ppc::protocol::MessagePayload::dataPtr;

/*
///// tests  ///
%inline {
}
///// tests  ///
*/

// define the interface should been exposed
%include "bcos-utilities/Error.h"
%include "ppc-framework/libwrapper/Buffer.h"
%include "ppc-framework/front/FrontConfig.h"
%include "ppc-framework/protocol/EndPoint.h"
%include "ppc-framework/protocol/GrpcConfig.h"
%include "ppc-framework/protocol/Message.h"
%include "ppc-framework/protocol/MessagePayload.h"

%include "ppc-framework/front/IFront.h"

%include "wedpr-transport/sdk/src/TransportBuilder.h"
%include "wedpr-transport/sdk/src/Transport.h"