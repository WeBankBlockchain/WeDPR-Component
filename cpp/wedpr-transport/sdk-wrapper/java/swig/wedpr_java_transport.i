%module(directors="1") wedpr_java_transport

%include "stdint.i"
%include "typemaps.i"

#if defined(SWIGJAVA)
#if defined(SWIGWORDSIZE64)
// By default SWIG map C++ long int (i.e. int64_t) to C# int
// But we want to map it to C# long so we reuse the typemap for C++ long long.
// ref: https://github.com/swig/swig/blob/master/Lib/java/typemaps.i
// ref: https://docs.oracle.com/en/java/javase/14/docs/api/java.base/java/lang/Long.html
%define PRIMITIVE_TYPEMAP(NEW_TYPE, TYPE)
%clear NEW_TYPE;
%clear NEW_TYPE *;
%clear NEW_TYPE &;
%clear const NEW_TYPE &;
%apply TYPE { NEW_TYPE };
%apply TYPE * { NEW_TYPE * };
%apply TYPE & { NEW_TYPE & };
%apply const TYPE & { const NEW_TYPE & };
%enddef // PRIMITIVE_TYPEMAP
PRIMITIVE_TYPEMAP(long int, long long);
PRIMITIVE_TYPEMAP(unsigned long int, long long);
#undef PRIMITIVE_TYPEMAP
#endif // defined(SWIGWORDSIZE64)
#endif // defined(SWIGJAVA)


%include <stdint.i>
%include <cpointer.i>
%include <std_vector.i>
%include <std_string.i>
%include <std_shared_ptr.i>
%include <various.i>

// shared_ptr definition
%shared_ptr(ppc::front::FrontConfig);
%shared_ptr(ppc::front::INodeDiscovery);
%shared_ptr(ppc::protocol::INodeInfo);
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
#include <vector>
#include <iostream>
#include <stdint.h>
#include "wedpr-transport/sdk/src/TransportBuilder.h"
#include "wedpr-transport/sdk/src/Transport.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-framework/front/INodeDiscovery.h"
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
    class INodeInfo;
}

namespace ppc::front{
    class FrontConfig;
    class IFront;
    class INodeDiscovery;
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
%template(SharedNodeDiscovery) std::shared_ptr<ppc::front::INodeDiscovery>;

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
%template(SharedNodeInfo) std::shared_ptr<ppc::protocol::INodeInfo>;

%template(ubytes) std::vector<uint8_t>;
%template(ibytes) std::vector<int8_t>;
%template(StringVec) std::vector<std::string>;
%template(NodeInfoVec) std::vector<std::shared_ptr<ppc::protocol::INodeInfo>>;

/// callbacks
%feature("director") ppc::front::ErrorCallback;
%feature("director") ppc::front::MessageDispatcherHandler;
%feature("director") ppc::front::IMessageHandler;
%feature("director") ppc::front::GetPeersInfoHandler;

// Note: the field data should equal to the fieldMap of class or the function
%include various.i 
// this means convert all (char*) to byte[]
%apply char *BYTE {char * };


%typemap(jni) OutputBuffer "jbyteArray"
%typemap(jtype) OutputBuffer "byte[]"
%typemap(jstype) OutputBuffer "byte[]"
%typemap(in) OutputBuffer {
    $1.data = (unsigned char *) JCALL2(GetByteArrayElements, jenv, $input, 0);
    $1.len = JCALL1(GetArrayLength, jenv, $input);
}
%typemap(argout) OutputBuffer {
    JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1.data, 0);
}
// Note: will cause copy here
%typemap(out) OutputBuffer {
    $result = JCALL1(NewByteArray, jenv, $1.len);
    JCALL4(SetByteArrayRegion, jenv, $result, 0, $1.len, (jbyte *) $1.data);
}
%typemap(javain) OutputBuffer "$javainput"
%typemap(javaout) OutputBuffer { return $jnicall; }



// refer to: https://stackoverflow.com/questions/12103206/is-it-possible-to-add-text-to-an-existing-typemap-in-swig
%define WRAP(CLASS)
%extend CLASS {
%proxycode %{
  public void disOwnMemory() {
    swigSetCMemOwn(false);
  }
%}
}
%enddef

// Note: these object is created from cpp, and maintained with shared_ptr, 
//        the java code should disOwnMemory in case of released multiple times
WRAP(ppc::front::FrontConfig)
WRAP(ppc::protocol::Message)
WRAP(ppc::protocol::MessageOptionalHeader)
WRAP(ppc::protocol::MessageHeader)
WRAP(ppc::protocol::MessagePayload)
WRAP(ppc::protocol::MessageBuilder)
WRAP(ppc::protocol::MessageHeaderBuilder)
WRAP(ppc::protocol::MessagePayloadBuilder)
WRAP(ppc::protocol::MessageOptionalHeaderBuilder)
WRAP(ppc::sdk::Transport)

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
%ignore ppc::protocol::MessageOptionalHeader::setSrcNode;
%ignore ppc::protocol::MessagePayload::data;
%ignore ppc::protocol::MessagePayload::setData;
%ignore ppc::protocol::MessagePayload::setDataPtr;
%ignore ppc::protocol::MessagePayload::dataPtr;
%ignore ppc::front::INodeDiscovery::start;
%ignore ppc::front::INodeDiscovery::stop;
%ignore ppc::protocol::INodeInfo::INodeInfo;
%ignore ppc::protocol::INodeInfo::setFront;
%ignore ppc::protocol::INodeInfo::getFront;
%ignore ppc::protocol::INodeInfo::components;
%ignore ppc::protocol::INodeInfo::encode;
%ignore ppc::protocol::INodeInfo::decode;
%ignore ppc::protocol::INodeInfo::equal;
%ignore ppc::protocol::INodeInfo::toJson;
%ignore ppc::protocol::INodeInfo::setComponents;
%ignore ppc::protocol::INodeInfoFactory;
%ignore ppc::protocol::Message::setFrontMessage;
%ignore ppc::protocol::GrpcConfig::~GrpcConfig;
%ignore ppc::protocol::GrpcServerConfig::~GrpcServerConfig;

%include "exception.i"
%exception {
    try {
        $action
    }
    catch (const std::exception& e) {
        SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, std::string(boost::diagnostic_information(e)).c_str());
        return $null;
    }
}

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
%include "ppc-framework/protocol/INodeInfo.h"

%include "ppc-framework/front/IFront.h"
%include "ppc-framework/front/INodeDiscovery.h"

%include "wedpr-transport/sdk/src/TransportBuilder.h"
%include "wedpr-transport/sdk/src/Transport.h"