#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <lumalink/platform/TransportFactory.h>
#ifdef _WIN32
#include <lumalink/platform/windows/WindowsSocketTransport.h>
#else
#include <lumalink/platform/posix/PosixSocketTransport.h>
#endif

#include <unity.h>

namespace
{
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::core::HttpStatus;
    using lumalink::http::handlers::HttpHandler;
    using lumalink::http::handlers::IHttpHandler;
    using lumalink::http::response::StringResponse;
    using lumalink::http::routing::HandlerMatcher;
    using lumalink::http::server::WebServer;
    using lumalink::platform::TransportFactory;

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_transport_factory_wrapper_exposes_static_factory_contract()
    {
        TEST_ASSERT_TRUE((lumalink::platform::transport::IsStaticTransportFactoryV<lumalink::platform::detail::NativeTransportFactory>));
    }

    void test_final_http_surface_constructs_server_through_platform_boundary()
    {
        TransportFactory transportFactory;
        auto transport = transportFactory.createServer(0);

        TEST_ASSERT_NOT_NULL(transport.get());

        WebServer server(std::move(transport));
        auto &config = server.cfg();
        HandlerMatcher matcher("/health", "GET");
        config.on(matcher, [](HttpRequestContext &) -> std::unique_ptr<IHttpHandler> {
            return std::make_unique<HttpHandler>(
                StringResponse::create(HttpStatus::Ok(), std::string_view("ok"), {}));
        });

        TEST_ASSERT_EQUAL_UINT32(0, static_cast<unsigned int>(server.activeConnections()));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_transport_factory_wrapper_exposes_static_factory_contract);
        RUN_TEST(test_final_http_surface_constructs_server_through_platform_boundary);
        return UNITY_END();
    }
}

int run_test_renamed_surface()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "renamed surface",
        runUnitySuite,
        localSetUp,
        localTearDown);
}