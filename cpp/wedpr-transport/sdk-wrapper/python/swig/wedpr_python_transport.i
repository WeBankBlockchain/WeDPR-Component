%module wedpr_python_transport

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
#include "wedpr-transport/sdk/src/TransportBuilder.h"
#include "wedpr-transport/sdk/src/Transport.h"
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

%include "wedpr-transport/sdk/src/TransportBuilder.h"
%include "wedpr-transport/sdk/src/Transport.h"
%include "ppc-framework/front/IFront.h"