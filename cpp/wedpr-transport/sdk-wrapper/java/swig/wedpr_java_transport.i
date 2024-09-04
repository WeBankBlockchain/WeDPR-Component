%module wedpr_java_transport

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


%shared_ptr(ppc::front::FrontConfig);
%shared_ptr(bcos::Error);
%shared_ptr(ppc::protocol::Message);
%shared_ptr(ppc::protocol::MessageOptionalHeader)
%shared_ptr(ppc::sdk::Transport);

%{
#define SWIG_FILE_WITH_INIT
#include <stdint.h>
#include "wedpr-transport/sdk/TransportBuilder.h"
#include "wedpr-transport/sdk/Transport.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/protocol/RouteType.h"
#include "ppc-framework/front/FrontConfig.h"
#include <bcos-utilities/Error.h>
%}

namespace ppc::front{
    class FrontConfig;
    class IFront;
    class FrontImpl;
    class FrontBuilderImpl;
    class RouteType;
    class GatewayEndPoint;
}

namespace ppc::sdk{
    class Transport;
    class TransportBuilder;
}

%template(SharedFrontConfig) std::shared_ptr<ppc::front::FrontConfig>;
%template(SharedBcosError) std::shared_ptr<bcos::Error>;
%template(SharedMessage) std::shared_ptr<ppc::protocol::Message>;
%template(SharedRouteInfo) std::shared_ptr<ppc::protocol::MessageOptionalHeader>;
%template(SharedTransport) std::shared_ptr<ppc::sdk::Transport>;

%include "wedpr-transport/sdk/TransportBuilder.h"
%include "wedpr-transport/sdk/Transport.h"
%include "ppc-framework/front/IFront.h"