#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/httpadv/v1/util/Clock.h"
#include "../../src/httpadv/v1/core/HttpHeader.h"
#include "../../src/httpadv/v1/core/HttpContext.h"
#include "../../src/httpadv/v1/handlers/HandlerTypes.h"
#include "../../src/httpadv/v1/routing/BasicAuthentication.h"
#include "../../src/httpadv/v1/routing/CrossOriginRequestSharing.h"
#include "../../src/httpadv/v1/routing/HandlerMatcher.h"
#include "../../src/httpadv/v1/routing/HandlerProviderRegistry.h"
#include "../../src/httpadv/v1/routing/ProviderRegistryBuilder.h"
#include "../../src/httpadv/v1/routing/ReplaceVariables.h"
#include "../../src/httpadv/v1/response/StringResponse.h"
#include "../../src/httpadv/v1/server/HttpServerBase.h"
#include "../../src/httpadv/v1/server/WebServerBuilder.h"
#include "../../src/httpadv/v1/server/WebServerConfig.h"
#include "../../src/httpadv/v1/websocket/WebSocketCallbacks.h"

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace httpadv::v1::core;
using namespace httpadv::v1::handlers;
using namespace httpadv::v1::pipeline;
using namespace httpadv::v1::response;
using namespace httpadv::v1::routing;
using namespace httpadv::v1::server;
using namespace httpadv::v1::transport;
using namespace lumalink::platform::buffers;
using namespace httpadv::v1::util;
using namespace httpadv::v1::websocket;

namespace
{
    std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string_view body, std::initializer_list<HttpHeader> headers = {});

    class NoOpHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpRequestContext &) override
        {
            return nullptr;
        }

        void handleBodyChunk(HttpRequestContext &, const uint8_t *, std::size_t) override
        {
        }
    };

    class RecordingHandler : public IHttpHandler
    {
    public:
        using StepCallback = std::function<HandlerResult(HttpRequestContext &)>;

        explicit RecordingHandler(StepCallback stepCallback = nullptr)
            : stepCallback_(std::move(stepCallback))
        {
        }

        HandlerResult handleStep(HttpRequestContext &context) override
        {
            ++handleStepCount_;
            if (stepCallback_)
            {
                return stepCallback_(context);
            }
            return nullptr;
        }

        void handleBodyChunk(HttpRequestContext &, const uint8_t *at, std::size_t length) override
        {
            ++bodyChunkCount_;
            body_.append(reinterpret_cast<const char *>(at), length);
        }

        std::size_t handleStepCount() const
        {
            return handleStepCount_;
        }

        std::size_t bodyChunkCount() const
        {
            return bodyChunkCount_;
        }

        const std::string &body() const
        {
            return body_;
        }

    private:
        StepCallback stepCallback_;
        std::size_t handleStepCount_ = 0;
        std::size_t bodyChunkCount_ = 0;
        std::string body_;
    };

    class StaticResponseProvider : public IHandlerProvider
    {
    public:
        explicit StaticResponseProvider(std::string responseBody, std::function<bool(HttpRequestContext &)> predicate = nullptr)
            : responseBody_(std::move(responseBody)),
              predicate_(std::move(predicate))
        {
        }

        bool canHandle(HttpRequestContext &context) override
        {
            ++canHandleCount_;
            return predicate_ ? predicate_(context) : true;
        }

        std::unique_ptr<IHttpHandler> create(HttpRequestContext &) override
        {
            ++createCount_;
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Ok(), responseBody_),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        }

        std::size_t canHandleCount() const
        {
            return canHandleCount_;
        }

        std::size_t createCount() const
        {
            return createCount_;
        }

    private:
        std::string responseBody_;
        std::function<bool(HttpRequestContext &)> predicate_;
        std::size_t canHandleCount_ = 0;
        std::size_t createCount_ = 0;
    };

    class RequestContextHarness
    {
    public:
        RequestContextHarness()
            : server_(std::make_unique<HttpServerBase>(std::make_unique<httpadv::v1::TestSupport::FakeServer>())),
              handler_(std::make_unique<NoOpHandler>()),
              factory_([this](HttpRequestContext &context) -> std::unique_ptr<IHttpHandler>
              {
                  context_ = static_cast<HttpContext *>(&context);
                  return std::move(handler_);
              }),
              pipeline_(HttpContext::createPipelineHandler(*server_, factory_))
        {
        }

        void start(std::string_view method, std::string_view url)
        {
            methodStorage_.assign(method.data(), method.size());
            urlStorage_.assign(url.data(), url.size());
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageBegin(methodStorage_.c_str(), 1, 1, urlStorage_));
        }

        void addHeader(std::string_view name, std::string_view value)
        {
            nameStorage_.assign(name.data(), name.size());
            valueStorage_.assign(value.data(), value.size());
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeader(nameStorage_, valueStorage_));
        }

        void completeHeaders()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeadersComplete());
        }

        HttpContext &context()
        {
            TEST_ASSERT_NOT_NULL(context_);
            return *context_;
        }

    private:
        std::unique_ptr<HttpServerBase> server_;
        std::unique_ptr<IHttpHandler> handler_;
        httpadv::v1::TestSupport::RecordingRequestHandlerFactory factory_;
        PipelineHandlerPtr pipeline_;
        HttpContext *context_ = nullptr;
        std::string methodStorage_;
        std::string urlStorage_;
        std::string nameStorage_;
        std::string valueStorage_;
    };

    struct AuthInvocationResult
    {
        bool nextCalled = false;
        std::optional<httpadv::v1::TestSupport::CapturedResponse> response;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string_view body, std::initializer_list<HttpHeader> headers)
    {
        return StringResponse::create(status, std::string(body), headers);
    }

    void prepareRequest(RequestContextHarness &harness, std::string_view method, std::string_view url, const std::vector<std::pair<std::string, std::string>> &headers = {})
    {
        harness.start(method, url);
        for (const auto &header : headers)
        {
            harness.addHeader(header.first, header.second);
        }
        harness.completeHeaders();
    }

    httpadv::v1::TestSupport::CapturedResponse captureHandlerResponse(IHttpHandler &handler, HttpContext &context)
    {
        return httpadv::v1::TestSupport::CaptureResponse(handler.handleStep(context));
    }

    AuthInvocationResult invokeAuth(IHttpHandler::InterceptorCallback interceptor, HttpContext &context)
    {
        AuthInvocationResult result;
        HandlerResult response = interceptor(context, IHttpHandler::InvocationNext(context, [&result]() -> HandlerResult
        {
            result.nextCalled = true;
            return HandlerResult::responseResult(createResponse(HttpStatus::Ok(), "authorized"));
        }));

        if (response.isResponse())
        {
            result.response = httpadv::v1::TestSupport::CaptureResponse(std::move(response));
        }

        return result;
    }

    void test_handler_matcher_covers_exact_wildcard_and_parameter_extraction()
    {
        const std::string filesWildcardPattern = "/files/:fileName";
        const std::string nestedWildcardPattern = "/users/:userId/photos/:photoId";
        const std::string globPattern = "**/*.html";
        const std::string mixedPattern = "/blogs/:blogId/**/*.html";

        RequestContextHarness exactHarness;
        prepareRequest(
            exactHarness,
            "GET",
            "/files/report.txt",
            {{"Content-Type", "Application/Json; charset=utf-8"}});
        HttpContext &exactContext = exactHarness.context();

        TEST_ASSERT_TRUE(defaultCheckUriPattern("/files/report.txt", "/files/report.txt"));
        TEST_ASSERT_TRUE(defaultCheckUriPattern("/files/report.txt", filesWildcardPattern));
        TEST_ASSERT_TRUE(defaultCheckMethod("GET,POST", "GET"));
        TEST_ASSERT_TRUE(defaultCheckContentType(exactContext, {"application/json"}));

        HandlerMatcher wildcardMatcher(filesWildcardPattern, "GET");
        TEST_ASSERT_TRUE(wildcardMatcher.canHandle(exactContext));

        const RouteParameters exactParams = wildcardMatcher.extractParameters(exactContext);
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(exactParams.size()));
        TEST_ASSERT_EQUAL_STRING("report.txt", exactParams.at("fileName").c_str());

        RequestContextHarness nestedHarness;
        prepareRequest(nestedHarness, "GET", "/users/42/photos/cover");
        HandlerMatcher nestedMatcher(nestedWildcardPattern, "GET");
        const RouteParameters nestedParams = nestedMatcher.extractParameters(nestedHarness.context());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(nestedParams.size()));
        TEST_ASSERT_EQUAL_STRING("42", nestedParams.at("userId").c_str());
        TEST_ASSERT_EQUAL_STRING("cover", nestedParams.at("photoId").c_str());

        RequestContextHarness globHarness;
        prepareRequest(globHarness, "GET", "/pages/docs/index.html");
        HandlerMatcher globMatcher(globPattern, "GET");
        TEST_ASSERT_TRUE(globMatcher.canHandle(globHarness.context()));

        const RouteParameters globParams = globMatcher.extractParameters(globHarness.context());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(globParams.size()));
        TEST_ASSERT_EQUAL_STRING("pages/docs", globParams.at("0").c_str());
        TEST_ASSERT_EQUAL_STRING("index.html", globParams.at("1").c_str());

        RequestContextHarness mixedHarness;
        prepareRequest(mixedHarness, "GET", "/blogs/alpha/posts/2026/launch.html");
        HandlerMatcher mixedMatcher(mixedPattern, "GET");
        TEST_ASSERT_TRUE(mixedMatcher.canHandle(mixedHarness.context()));

        const RouteParameters mixedParams = mixedMatcher.extractParameters(mixedHarness.context());
        TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(mixedParams.size()));
        TEST_ASSERT_EQUAL_STRING("alpha", mixedParams.at("blogId").c_str());
        TEST_ASSERT_EQUAL_STRING("posts/2026", mixedParams.at("0").c_str());
        TEST_ASSERT_EQUAL_STRING("launch.html", mixedParams.at("1").c_str());
    }

    void test_handler_matcher_rejects_mismatched_methods_content_types_and_missing_metadata()
    {
        const std::string filesWildcardPattern = "/files/:fileName";
        const std::string assetsWildcardPattern = "/assets/:assetName";
        const std::string htmlGlobPattern = "**/*.html";

        RequestContextHarness jsonHarness;
        prepareRequest(
            jsonHarness,
            "POST",
            "/files/report.txt",
            {{"Content-Type", "text/plain"}});
        HttpContext &jsonContext = jsonHarness.context();

        HandlerMatcher methodMatcher(filesWildcardPattern, "GET");
        TEST_ASSERT_FALSE(methodMatcher.canHandle(jsonContext));

        HandlerMatcher contentTypeMatcher(filesWildcardPattern, "POST", {"application/json"});
        TEST_ASSERT_FALSE(contentTypeMatcher.canHandle(jsonContext));

        RequestContextHarness missingContentTypeHarness;
        prepareRequest(missingContentTypeHarness, "POST", "/files/report.txt");
        TEST_ASSERT_FALSE(contentTypeMatcher.canHandle(missingContentTypeHarness.context()));

        RequestContextHarness missingMethodHarness;
        prepareRequest(missingMethodHarness, "", "/files/report.txt", {{"Content-Type", "application/json"}});
        TEST_ASSERT_FALSE(methodMatcher.canHandle(missingMethodHarness.context()));

        TEST_ASSERT_FALSE(defaultCheckUriPattern("/files/report.txt", assetsWildcardPattern));
        TEST_ASSERT_FALSE(defaultCheckUriPattern("/files/report.txt", htmlGlobPattern));
    }

    void test_handler_provider_registry_honors_beginning_and_end_order_and_custom_default_handler()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/registry/order");
        HttpContext &context = harness.context();

        StaticResponseProvider beginningProvider("beginning");
        StaticResponseProvider firstEndProvider("first-end", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-End-Route", "first");
        });
        StaticResponseProvider secondEndProvider("second-end");

        HandlerProviderRegistry registry;
        registry.add(firstEndProvider, HandlerProviderRegistry::AddAt::End);
        registry.add(secondEndProvider, HandlerProviderRegistry::AddAt::End);
        registry.add(beginningProvider, HandlerProviderRegistry::AddAt::Beginning);

        auto beginningHandler = registry.createContextHandler(context);
        const auto beginningResponse = captureHandlerResponse(*beginningHandler, context);
        TEST_ASSERT_EQUAL_STRING("beginning", beginningResponse.body.c_str());
        TEST_ASSERT_EQUAL_UINT64(1, beginningProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, beginningProvider.createCount());
        TEST_ASSERT_EQUAL_UINT64(0, firstEndProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(0, secondEndProvider.canHandleCount());

        RequestContextHarness endHarness;
        prepareRequest(endHarness, "GET", "/registry/order", {{"X-End-Route", "first"}});
        HandlerProviderRegistry endRegistry;
        StaticResponseProvider nonMatchingProvider("never", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-End-Route", "missing");
        });
        StaticResponseProvider matchingEndProvider("matched-end", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-End-Route", "first");
        });
        endRegistry.add(nonMatchingProvider, HandlerProviderRegistry::AddAt::End);
        endRegistry.add(matchingEndProvider, HandlerProviderRegistry::AddAt::End);
        endRegistry.setDefaultHandlerFactory([](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::NotFound(), "custom-default"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        auto endHandler = endRegistry.createContextHandler(endHarness.context());
        const auto endResponse = captureHandlerResponse(*endHandler, endHarness.context());
        TEST_ASSERT_EQUAL_STRING("matched-end", endResponse.body.c_str());
        TEST_ASSERT_EQUAL_UINT64(1, nonMatchingProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, matchingEndProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, matchingEndProvider.createCount());

        RequestContextHarness defaultHarness;
        prepareRequest(defaultHarness, "GET", "/registry/default");
        auto defaultHandler = endRegistry.createContextHandler(defaultHarness.context());
        const auto defaultResponse = captureHandlerResponse(*defaultHandler, defaultHarness.context());
        TEST_ASSERT_EQUAL_STRING("custom-default", defaultResponse.body.c_str());
    }

    void test_handler_provider_registry_applies_filters_interceptors_and_body_forwarding_for_matches()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "POST", "/registry/filtered", {{"X-Allow", "yes"}});
        HttpContext &context = harness.context();

        HandlerProviderRegistry registry;
        std::vector<std::string> trace;
        RecordingHandler *innerHandler = nullptr;

        registry.filterRequest([](HttpRequestContext &request)
        {
            return request.methodView() == "POST";
        });
        registry.filterRequest([](HttpRequestContext &request)
        {
            return request.headers().exists("X-Allow", "yes");
        });

        registry.with([&trace](HttpRequestContext &request, IHttpHandler::InvocationNext next) -> IHttpHandler::HandlerResult
        {
            trace.push_back("interceptor-1-before");
            auto response = next();
            trace.push_back("interceptor-1-after");
            return response;
        });
        registry.with([&trace](HttpRequestContext &request, IHttpHandler::InvocationNext next) -> IHttpHandler::HandlerResult
        {
            trace.push_back("interceptor-2-before");
            auto response = next();
            trace.push_back("interceptor-2-after");
            return response;
        });

        registry.apply([&trace](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
        {
            trace.push_back("filter-1");
            response->headers().set("X-Filter", "one");
            return response;
        });
        registry.apply([&trace](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
        {
            trace.push_back("filter-2");
            response->headers().set("X-Filter", "two", false);
            return response;
        });

        registry.add(
            [](HttpRequestContext &)
            {
                return true;
            },
            [&innerHandler, &trace](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
            {
                auto handler = std::make_unique<RecordingHandler>([&trace](HttpRequestContext &) -> std::unique_ptr<IHttpResponse>
                {
                    trace.push_back("handler");
                    return createResponse(HttpStatus::Ok(), "matched");
                });
                innerHandler = handler.get();
                return handler;
            });

        auto handler = registry.createContextHandler(context);
        TEST_ASSERT_NOT_NULL(innerHandler);

        const std::uint8_t bodyChunk[] = {'a', 'b', 'c'};
        handler->handleBodyChunk(context, bodyChunk, sizeof(bodyChunk));

        const auto response = captureHandlerResponse(*handler, context);
        TEST_ASSERT_EQUAL_STRING("matched", response.body.c_str());
        TEST_ASSERT_EQUAL_UINT64(1, innerHandler->bodyChunkCount());
        TEST_ASSERT_EQUAL_STRING("abc", innerHandler->body().c_str());

        const auto filterHeader = httpadv::v1::TestSupport::FindCapturedHeader(response, "X-Filter");
        TEST_ASSERT_TRUE(filterHeader.has_value());
        TEST_ASSERT_EQUAL_STRING("one,two", filterHeader->c_str());

        TEST_ASSERT_EQUAL_UINT32(7, static_cast<uint32_t>(trace.size()));
        TEST_ASSERT_EQUAL_STRING("interceptor-2-before", trace[0].c_str());
        TEST_ASSERT_EQUAL_STRING("interceptor-1-before", trace[1].c_str());
        TEST_ASSERT_EQUAL_STRING("handler", trace[2].c_str());
        TEST_ASSERT_EQUAL_STRING("interceptor-1-after", trace[3].c_str());
        TEST_ASSERT_EQUAL_STRING("interceptor-2-after", trace[4].c_str());
        TEST_ASSERT_EQUAL_STRING("filter-1", trace[5].c_str());
        TEST_ASSERT_EQUAL_STRING("filter-2", trace[6].c_str());

        RequestContextHarness blockedHarness;
        prepareRequest(blockedHarness, "POST", "/registry/filtered");
        registry.setDefaultHandlerFactory([](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Forbidden(), "blocked"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        auto blockedHandler = registry.createContextHandler(blockedHarness.context());
        const auto blockedResponse = captureHandlerResponse(*blockedHandler, blockedHarness.context());
        TEST_ASSERT_EQUAL_UINT16(403, blockedResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("blocked", blockedResponse.body.c_str());
    }

    void test_handler_provider_registry_response_filters_only_run_for_non_null_responses()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/registry/null-response");
        HttpContext &context = harness.context();

        HandlerProviderRegistry registry;
        int filterCallCount = 0;
        registry.apply([&filterCallCount](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
        {
            ++filterCallCount;
            return response;
        });

        registry.add(
            [](HttpRequestContext &)
            {
                return true;
            },
            [](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
            {
                return std::make_unique<RecordingHandler>();
            });

        auto handler = registry.createContextHandler(context);
        const auto response = handler->handleStep(context);
        TEST_ASSERT_NULL(response.get());
        TEST_ASSERT_EQUAL_INT(0, filterCallCount);
    }

    void test_provider_registry_builder_on_factory_overload_registers_for_builder_and_web_server_config()
    {
        RequestContextHarness builderHarness;
        prepareRequest(builderHarness, "GET", "/legacy/path");

        HandlerProviderRegistry builderRegistry;
        ProviderRegistryBuilder builder(builderRegistry);
        const std::string builderPattern = "/legacy/:resource";
        HandlerMatcher builderMatcher(builderPattern, "GET");
        std::size_t builderFactoryCalls = 0;
        builder.on(builderMatcher, [&builderFactoryCalls](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            ++builderFactoryCalls;
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Ok(), "builder-on"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        auto builderHandler = builderRegistry.createContextHandler(builderHarness.context());
        const auto builderResponse = captureHandlerResponse(*builderHandler, builderHarness.context());
        TEST_ASSERT_EQUAL_UINT64(1, builderFactoryCalls);
        TEST_ASSERT_EQUAL_STRING("builder-on", builderResponse.body.c_str());

        RequestContextHarness configHarness;
        prepareRequest(configHarness, "GET", "/config/path");

        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);
        const std::string configPattern = "/config/:resource";
        HandlerMatcher configMatcher(configPattern, "GET");
        std::size_t configFactoryCalls = 0;
        config.on(configMatcher, [&configFactoryCalls](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            ++configFactoryCalls;
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Ok(), "config-on"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        auto configHandler = config.handlerProviders().createContextHandler(configHarness.context());
        const auto configResponse = captureHandlerResponse(*configHandler, configHarness.context());
        TEST_ASSERT_EQUAL_UINT64(1, configFactoryCalls);
        TEST_ASSERT_EQUAL_STRING("config-on", configResponse.body.c_str());

        RequestContextHarness nonMatchHarness;
        prepareRequest(nonMatchHarness, "GET", "/unmatched");
        auto nonMatchHandler = config.handlerProviders().createContextHandler(nonMatchHarness.context());
        const auto nonMatchResponse = captureHandlerResponse(*nonMatchHandler, nonMatchHarness.context());
        TEST_ASSERT_EQUAL_UINT16(404, nonMatchResponse.status.code());
    }

    void test_provider_registry_builder_websocket_registration_creates_upgrade_handler()
    {
        HandlerProviderRegistry registry;
        ProviderRegistryBuilder builder(registry);

        bool openCalled = false;
        WebSocketCallbacks callbacks;
        callbacks.onOpen = [&openCalled](WebSocketContext &)
        {
            openCalled = true;
        };

        const std::string websocketPattern = "/ws/:channel";
        builder.websocket(websocketPattern, callbacks);

        RequestContextHarness harness;
        prepareRequest(
            harness,
            "GET",
            "/ws/chat",
            {
                {"Host", "example.test"},
                {"Connection", "Upgrade"},
                {"Upgrade", "websocket"},
                {"Sec-WebSocket-Version", "13"},
                {"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="}
            });

        auto handler = registry.createContextHandler(harness.context());
        HandlerResult result = handler->handleStep(harness.context());

        TEST_ASSERT_TRUE(result.isUpgrade());
        TEST_ASSERT_NOT_NULL(result.upgradedSession.get());

        httpadv::v1::TestSupport::FakeClient client;
        ManualClock clock(1000);
        const ConnectionSessionResult firstStep = result.upgradedSession->handle(client, clock);

        TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionSessionResult::Continue), static_cast<int>(firstStep));
        TEST_ASSERT_TRUE(openCalled);
    }

    void test_handler_provider_registry_supports_indexed_insertion_and_out_of_range_clamping()
    {
        StaticResponseProvider firstProvider("first", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-Match", "first");
        });
        StaticResponseProvider secondProvider("second", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-Match", "second");
        });
        StaticResponseProvider indexedProvider("indexed", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-Match", "indexed");
        });

        HandlerProviderRegistry indexedRegistry;
        indexedRegistry.add(firstProvider, HandlerProviderRegistry::AddAt::End);
        indexedRegistry.add(secondProvider, HandlerProviderRegistry::AddAt::End);
        indexedRegistry.add(indexedProvider, 1);

        RequestContextHarness hitIndexedHarness;
        prepareRequest(hitIndexedHarness, "GET", "/registry/indexed", {{"X-Match", "indexed"}});
        auto indexedHandler = indexedRegistry.createContextHandler(hitIndexedHarness.context());
        const auto indexedResponse = captureHandlerResponse(*indexedHandler, hitIndexedHarness.context());
        TEST_ASSERT_EQUAL_STRING("indexed", indexedResponse.body.c_str());
        TEST_ASSERT_EQUAL_UINT64(1, firstProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(1, indexedProvider.canHandleCount());
        TEST_ASSERT_EQUAL_UINT64(0, secondProvider.canHandleCount());

        StaticResponseProvider endFirst("end-first", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-Match", "end-first");
        });
        StaticResponseProvider endLast("end-last", [](HttpRequestContext &request)
        {
            return request.headers().exists("X-Match", "end-last");
        });

        HandlerProviderRegistry clampedRegistry;
        clampedRegistry.add(endFirst, HandlerProviderRegistry::AddAt::End);
        clampedRegistry.add(endLast, 99);

        RequestContextHarness hitFirstHarness;
        prepareRequest(hitFirstHarness, "GET", "/registry/clamped", {{"X-Match", "end-first"}});
        auto firstHandler = clampedRegistry.createContextHandler(hitFirstHarness.context());
        const auto firstResponse = captureHandlerResponse(*firstHandler, hitFirstHarness.context());
        TEST_ASSERT_EQUAL_STRING("end-first", firstResponse.body.c_str());

        RequestContextHarness hitLastHarness;
        prepareRequest(hitLastHarness, "GET", "/registry/clamped", {{"X-Match", "end-last"}});
        auto lastHandler = clampedRegistry.createContextHandler(hitLastHarness.context());
        const auto lastResponse = captureHandlerResponse(*lastHandler, hitLastHarness.context());
        TEST_ASSERT_EQUAL_STRING("end-last", lastResponse.body.c_str());
    }

    void test_handler_provider_registry_uses_default_not_found_response_when_no_match_exists()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/missing");

        HandlerProviderRegistry registry;
        auto handler = registry.createContextHandler(harness.context());
        const auto response = captureHandlerResponse(*handler, harness.context());

        TEST_ASSERT_EQUAL_UINT16(404, response.status.code());
        TEST_ASSERT_EQUAL_STRING("404 Not Found", response.body.c_str());
    }

    void test_handler_matcher_mutators_override_runtime_checker_and_extractor_behavior()
    {
        RequestContextHarness harness;
        prepareRequest(
            harness,
            "PATCH",
            "/custom/path?mode=test",
            {{"Content-Type", "text/plain"}, {"X-Custom", "yes"}});

        HandlerMatcher matcher("/initial", "GET", {"application/json"});
        matcher.setUriPattern("/custom/:segment");
        matcher.setAllowedMethods("PATCH");
        matcher.setAllowedContentTypes({"text/plain"});

        std::size_t methodChecks = 0;
        std::size_t uriChecks = 0;
        std::size_t contentTypeChecks = 0;
        std::size_t extractCalls = 0;

        matcher.setMethodChecker([&methodChecks](std::string_view allowedMethods, std::string_view method)
        {
            ++methodChecks;
            TEST_ASSERT_EQUAL_STRING("PATCH", std::string(allowedMethods).c_str());
            TEST_ASSERT_EQUAL_STRING("PATCH", std::string(method).c_str());
            return true;
        });
        matcher.setUriPatternChecker([&uriChecks](std::string_view uri, std::string_view pattern)
        {
            ++uriChecks;
            TEST_ASSERT_EQUAL_STRING("/custom/:segment", std::string(pattern).c_str());
            return uri.find("/custom/path") != std::string_view::npos;
        });
        matcher.setContentTypeChecker([&contentTypeChecks](HttpRequestContext &request, const std::vector<std::string> &allowedContentTypes)
        {
            ++contentTypeChecks;
            TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(allowedContentTypes.size()));
            return request.headers().exists("X-Custom", "yes");
        });
        matcher.setArgsExtractor([&extractCalls](HttpRequestContext &, std::string_view pattern)
        {
            ++extractCalls;
            TEST_ASSERT_EQUAL_STRING("/custom/:segment", std::string(pattern).c_str());
            return RouteParameters{{"segment", "value"}};
        });

        TEST_ASSERT_TRUE(matcher.canHandle(harness.context()));
        const RouteParameters params = matcher.extractParameters(harness.context());
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(params.size()));
        TEST_ASSERT_EQUAL_STRING("value", params.at("segment").c_str());
        TEST_ASSERT_EQUAL_UINT64(1, methodChecks);
        TEST_ASSERT_EQUAL_UINT64(1, uriChecks);
        TEST_ASSERT_EQUAL_UINT64(1, contentTypeChecks);
        TEST_ASSERT_EQUAL_UINT64(1, extractCalls);
    }

    void test_default_uri_pattern_matching_uses_path_and_ignores_query_string_with_wildcards()
    {
        const std::string pattern = "/docs/:slug";
        TEST_ASSERT_TRUE(defaultCheckUriPattern("/docs/readme?version=1", pattern));
        TEST_ASSERT_FALSE(defaultCheckUriPattern("/doc/readme?version=1", pattern));

        const std::string htmlGlobPattern = "**/*.html";
        TEST_ASSERT_TRUE(defaultCheckUriPattern("/guides/http/intro.html?version=1", htmlGlobPattern));
        TEST_ASSERT_FALSE(defaultCheckUriPattern("/guides/http/intro.json?version=1", htmlGlobPattern));
    }

    void test_handler_provider_registry_ignores_null_callbacks_in_composition_chain()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/registry/null-callbacks");

        HandlerProviderRegistry registry;
        registry.filterRequest(nullptr);
        registry.with(nullptr);
        registry.apply(nullptr);
        registry.add(
            [](HttpRequestContext &)
            {
                return true;
            },
            [](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
            {
                return std::make_unique<HttpHandler>(
                    createResponse(HttpStatus::Ok(), "safe"),
                    [](const HttpRequestContext &)
                    {
                        return true;
                    });
            });

        auto handler = registry.createContextHandler(harness.context());
        const auto response = captureHandlerResponse(*handler, harness.context());
        TEST_ASSERT_EQUAL_UINT16(200, response.status.code());
        TEST_ASSERT_EQUAL_STRING("safe", response.body.c_str());
    }

    void test_handler_builder_composes_matchers_predicates_interceptors_and_filters()
    {
        HandlerProviderRegistry registry;
        registry.setDefaultHandlerFactory([](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::NotFound(), "builder-default"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        ProviderRegistryBuilder builder(registry);
        {
            HandlerMatcher matcher(
                "/docs/:slug",
                defaultCheckMethod,
                [](std::string_view, std::string_view)
                {
                    return true;
                },
                defaultCheckContentType,
                [](HttpRequestContext &, std::string_view)
                {
                    return RouteParameters{{"slug", "readme"}};
                },
                "get",
                {"Application/Json"});

            auto route = builder.on<GetRequest>(matcher, [](HttpRequestContext &request, RouteParameters &&params) -> std::unique_ptr<IHttpResponse>
            {
                TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(params.size()));
                request.items()["route-param"] = params.at("slug");
                return createResponse(HttpStatus::Ok(), std::string("doc:") + params.at("slug"));
            });

            route.filterRequest([](HttpRequestContext &request)
                {
                    return request.headers().exists("X-Route", "yes");
                })
                .filterRequest([](HttpRequestContext &request)
                {
                    return !request.headers().exists("X-Block", "true");
                })
                .allowMethods("get")
                .allowContentTypes({"Application/Json"})
                .with([](HttpRequestContext &request, IHttpHandler::InvocationNext next) -> IHttpHandler::HandlerResult
                {
                    request.items()["builder-interceptor"] = true;
                    return next();
                })
                .apply([](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
                {
                    response->headers().set("X-Builder-Filter", "applied");
                    return response;
                });
        }

        RequestContextHarness matchedHarness;
        prepareRequest(
            matchedHarness,
            "GET",
            "/docs/readme",
            {{"X-Route", "yes"}, {"Content-Type", "application/json; charset=utf-8"}});
        HttpContext &matchedContext = matchedHarness.context();

        auto matchedHandler = registry.createContextHandler(matchedContext);
        const auto matchedResponse = captureHandlerResponse(*matchedHandler, matchedContext);
        TEST_ASSERT_EQUAL_STRING("doc:readme", matchedResponse.body.c_str());
        TEST_ASSERT_TRUE(std::any_cast<bool>(matchedContext.items().at("builder-interceptor")));

        const auto routeParam = std::any_cast<std::string>(&matchedContext.items().at("route-param"));
        TEST_ASSERT_NOT_NULL(routeParam);
        TEST_ASSERT_EQUAL_STRING("readme", routeParam->c_str());

        const auto builderFilter = httpadv::v1::TestSupport::FindCapturedHeader(matchedResponse, "X-Builder-Filter");
        TEST_ASSERT_TRUE(builderFilter.has_value());
        TEST_ASSERT_EQUAL_STRING("applied", builderFilter->c_str());

        RequestContextHarness blockedHarness;
        prepareRequest(
            blockedHarness,
            "POST",
            "/docs/readme",
            {{"X-Route", "yes"}, {"Content-Type", "application/json"}});
        auto blockedHandler = registry.createContextHandler(blockedHarness.context());
        const auto blockedResponse = captureHandlerResponse(*blockedHandler, blockedHarness.context());
        TEST_ASSERT_EQUAL_STRING("builder-default", blockedResponse.body.c_str());

        RequestContextHarness missingHeaderHarness;
        prepareRequest(
            missingHeaderHarness,
            "GET",
            "/docs/readme",
            {{"Content-Type", "application/json"}});
        auto missingHeaderHandler = registry.createContextHandler(missingHeaderHarness.context());
        const auto missingHeaderResponse = captureHandlerResponse(*missingHeaderHandler, missingHeaderHarness.context());
        TEST_ASSERT_EQUAL_STRING("builder-default", missingHeaderResponse.body.c_str());
    }

    void test_handler_registration_accepts_abstract_request_context_callbacks()
    {
        HandlerProviderRegistry registry;
        registry.setDefaultHandlerFactory([](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::NotFound(), "base-default"),
                [](const HttpRequestContext &)
                {
                    return true;
                });
        });

        ProviderRegistryBuilder builder(registry);
        const std::string builderPattern = "/base/:slug";
        {
            auto route = builder.on<GetRequest>(builderPattern.c_str(), [](HttpRequestContext &request, RouteParameters &&params) -> std::unique_ptr<IHttpResponse>
            {
                request.items()["base-route-param"] = params.at("slug");
                request.items()["base-method"] = std::string(request.methodView());
                return StringResponse::create(HttpStatus::Ok(), std::string("base:") + params.at("slug"), {});
            });

            route.filterRequest([](HttpRequestContext &request)
            {
                return request.headers().exists("X-Allow", "yes");
            });
        }

        RequestContextHarness matchedHarness;
        prepareRequest(matchedHarness, "GET", "/base/demo", {{"X-Allow", "yes"}});
        HttpContext &matchedContext = matchedHarness.context();

        auto matchedHandler = registry.createContextHandler(matchedContext);
        const auto matchedResponse = captureHandlerResponse(*matchedHandler, matchedContext);
        TEST_ASSERT_EQUAL_STRING("base:demo", matchedResponse.body.c_str());

        const auto routeParam = std::any_cast<std::string>(&matchedContext.items().at("base-route-param"));
        TEST_ASSERT_NOT_NULL(routeParam);
        TEST_ASSERT_EQUAL_STRING("demo", routeParam->c_str());

        const auto method = std::any_cast<std::string>(&matchedContext.items().at("base-method"));
        TEST_ASSERT_NOT_NULL(method);
        TEST_ASSERT_EQUAL_STRING("GET", method->c_str());

        RequestContextHarness blockedHarness;
        prepareRequest(blockedHarness, "GET", "/base/demo");
        auto blockedHandler = registry.createContextHandler(blockedHarness.context());
        const auto blockedResponse = captureHandlerResponse(*blockedHandler, blockedHarness.context());
        TEST_ASSERT_EQUAL_UINT16(404, blockedResponse.status.code());

        RequestContextHarness configHarness;
        prepareRequest(configHarness, "GET", "/config-base/sample");

        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);
        const std::string configPattern = "/config-base/:slug";
        config.on<GetRequest>(configPattern.c_str(), [](HttpRequestContext &request, RouteParameters &&params) -> std::unique_ptr<IHttpResponse>
        {
            request.items()["config-route-param"] = params.at("slug");
            return StringResponse::create(HttpStatus::Ok(), std::string("config:") + params.at("slug"), {});
        });

        auto configHandler = config.handlerProviders().createContextHandler(configHarness.context());
        const auto configResponse = captureHandlerResponse(*configHandler, configHarness.context());
        TEST_ASSERT_EQUAL_STRING("config:sample", configResponse.body.c_str());

        const auto configRouteParam = std::any_cast<std::string>(&configHarness.context().items().at("config-route-param"));
        TEST_ASSERT_NOT_NULL(configRouteParam);
        TEST_ASSERT_EQUAL_STRING("sample", configRouteParam->c_str());
    }

    void test_basic_auth_rejects_missing_malformed_and_separatorless_credentials_with_challenge_header()
    {
        auto auth = BasicAuth("admin", "secret", "Admin Area");

        RequestContextHarness missingHeaderHarness;
        prepareRequest(missingHeaderHarness, "GET", "/admin");
        const auto missingHeaderResult = invokeAuth(auth, missingHeaderHarness.context());
        TEST_ASSERT_FALSE(missingHeaderResult.nextCalled);
        TEST_ASSERT_TRUE(missingHeaderResult.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(401, missingHeaderResult.response->status.code());
        const auto missingHeaderChallenge = httpadv::v1::TestSupport::FindCapturedHeader(*missingHeaderResult.response, HttpHeaderNames::WwwAuthenticate);
        TEST_ASSERT_TRUE(missingHeaderChallenge.has_value());
        TEST_ASSERT_EQUAL_STRING("Basic realm=\"Admin Area\"", missingHeaderChallenge->c_str());

        RequestContextHarness malformedPrefixHarness;
        prepareRequest(malformedPrefixHarness, "GET", "/admin", {{"Authorization", "Bearer dXNlcjpwYXNz"}});
        const auto malformedPrefixResult = invokeAuth(auth, malformedPrefixHarness.context());
        TEST_ASSERT_FALSE(malformedPrefixResult.nextCalled);
        TEST_ASSERT_TRUE(malformedPrefixResult.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(401, malformedPrefixResult.response->status.code());

        RequestContextHarness malformedBase64Harness;
        prepareRequest(malformedBase64Harness, "GET", "/admin", {{"Authorization", "Basic !!!!"}});
        const auto malformedBase64Result = invokeAuth(auth, malformedBase64Harness.context());
        TEST_ASSERT_FALSE(malformedBase64Result.nextCalled);
        TEST_ASSERT_TRUE(malformedBase64Result.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(401, malformedBase64Result.response->status.code());

        RequestContextHarness missingSeparatorHarness;
        prepareRequest(missingSeparatorHarness, "GET", "/admin", {{"Authorization", "Basic bm9zZXBhcmF0b3I="}});
        const auto missingSeparatorResult = invokeAuth(auth, missingSeparatorHarness.context());
        TEST_ASSERT_FALSE(missingSeparatorResult.nextCalled);
        TEST_ASSERT_TRUE(missingSeparatorResult.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(401, missingSeparatorResult.response->status.code());
    }

    void test_basic_auth_accepts_valid_credentials_for_fixed_and_validator_overloads()
    {
        auto fixedAuth = BasicAuth("admin", "secret", "Admin Area");

        RequestContextHarness fixedHarness;
        prepareRequest(fixedHarness, "GET", "/admin", {{"Authorization", "Basic YWRtaW46c2VjcmV0"}});
        const auto fixedResult = invokeAuth(fixedAuth, fixedHarness.context());
        TEST_ASSERT_TRUE(fixedResult.nextCalled);
        TEST_ASSERT_TRUE(fixedResult.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(200, fixedResult.response->status.code());

        const auto storedUsername = std::any_cast<std::string>(&fixedHarness.context().items().at("BasicAuth::Username"));
        const auto storedPassword = std::any_cast<std::string>(&fixedHarness.context().items().at("BasicAuth::Password"));
        TEST_ASSERT_NOT_NULL(storedUsername);
        TEST_ASSERT_NOT_NULL(storedPassword);
        TEST_ASSERT_EQUAL_STRING("admin", storedUsername->c_str());
        TEST_ASSERT_EQUAL_STRING("secret", storedPassword->c_str());

        std::string expectedUsername = "service";
        std::string expectedPassword = "token";
        std::string capturedUsername;
        std::string capturedPassword;
        auto validatorAuth = BasicAuth(
            [&expectedUsername, &expectedPassword](std::string_view username, std::string_view password)
            {
                return username == expectedUsername && password == expectedPassword;
            },
            "Service Area",
            [&capturedUsername, &capturedPassword](std::string_view username, std::string_view password)
            {
                capturedUsername.assign(username.data(), username.size());
                capturedPassword.assign(password.data(), password.size());
            });

        RequestContextHarness validatorHarness;
        prepareRequest(validatorHarness, "GET", "/service", {{"Authorization", "Basic c2VydmljZTp0b2tlbg=="}});
        const auto validatorResult = invokeAuth(validatorAuth, validatorHarness.context());
        TEST_ASSERT_TRUE(validatorResult.nextCalled);
        TEST_ASSERT_TRUE(validatorResult.response.has_value());
        TEST_ASSERT_EQUAL_STRING("service", capturedUsername.c_str());
        TEST_ASSERT_EQUAL_STRING("token", capturedPassword.c_str());
    }

    void test_cors_omits_optional_headers_and_repeated_application_preserves_existing_values()
    {
        auto response = createResponse(HttpStatus::Ok(), "ok");
        auto firstFilter = CrossOriginRequestSharing("https://first.example", "GET", "X-First");
        response = firstFilter(std::move(response));

        const auto firstAllowOrigin = response->headers().find(HttpHeaderNames::AccessControlAllowOrigin);
        const auto firstAllowMethods = response->headers().find(HttpHeaderNames::AccessControlAllowMethods);
        const auto firstAllowHeaders = response->headers().find(HttpHeaderNames::AccessControlAllowHeaders);
        TEST_ASSERT_TRUE(firstAllowOrigin.has_value());
        TEST_ASSERT_TRUE(firstAllowMethods.has_value());
        TEST_ASSERT_TRUE(firstAllowHeaders.has_value());
        TEST_ASSERT_EQUAL_STRING("https://first.example", std::string(firstAllowOrigin->valueView()).c_str());
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::AccessControlAllowCredentials));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::AccessControlExposeHeaders));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::AccessControlMaxAge));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::AccessControlRequestHeaders));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::AccessControlRequestMethod));

        auto secondFilter = CrossOriginRequestSharing("https://second.example", "POST", "X-Second", "true", "X-Expose", 300, "X-Request", "OPTIONS");
        response = secondFilter(std::move(response));

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowOrigin, "https://first.example"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowMethods, "GET"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowHeaders, "X-First"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowCredentials, "true"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlExposeHeaders, "X-Expose"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlMaxAge, "300"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlRequestHeaders, "X-Request"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlRequestMethod, "OPTIONS"));
    }

    void test_cors_does_not_overwrite_existing_headers()
    {
        auto response = StringResponse::create(
            HttpStatus::Ok(),
            std::string("ok"),
            {
                HttpHeader(HttpHeaderNames::AccessControlAllowOrigin, "preset-origin"),
                HttpHeader(HttpHeaderNames::AccessControlMaxAge, "10")
            });

        auto filter = CrossOriginRequestSharing("https://replacement.example", "POST", "X-Test", "true", "X-Expose", 60);
        response = filter(std::move(response));

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowOrigin, "preset-origin"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlMaxAge, "10"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowMethods, "POST"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::AccessControlAllowHeaders, "X-Test"));
    }

    void test_cors_handling_component_can_be_applied_through_web_server_config_use()
    {
        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);

        config.use(CorsHandling("https://api.example", "GET, OPTIONS", "X-Test"));

        HandlerMatcher corsMatcher("/cors", "GET");
        config.add(
            [corsMatcher](HttpRequestContext &context) mutable
            {
                return corsMatcher.canHandle(context);
            },
            [](HttpRequestContext &) -> HandlerResult
            {
                return HandlerResult::responseResult(createResponse(HttpStatus::Ok(), "cors-ok"));
            });

        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/cors");

        auto handler = config.handlerProviders().createContextHandler(harness.context());
        const auto capturedResponse = captureHandlerResponse(*handler, harness.context());

        TEST_ASSERT_EQUAL_UINT16(200, capturedResponse.status.code());
        TEST_ASSERT_EQUAL_STRING("cors-ok", capturedResponse.body.c_str());
        const auto allowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowOrigin);
        const auto allowMethods = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowMethods);
        const auto allowHeaders = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowHeaders);

        TEST_ASSERT_TRUE(allowOrigin.has_value());
        TEST_ASSERT_TRUE(allowMethods.has_value());
        TEST_ASSERT_TRUE(allowHeaders.has_value());
        TEST_ASSERT_EQUAL_STRING("https://api.example", allowOrigin->c_str());
        TEST_ASSERT_EQUAL_STRING("GET, OPTIONS", allowMethods->c_str());
        TEST_ASSERT_EQUAL_STRING("X-Test", allowHeaders->c_str());
    }

    void test_cors_handling_component_auto_handles_preflight_options_without_manual_route()
    {
        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);

        config.use(CorsHandling("https://api.example", "GET, OPTIONS", "X-Test"));

        RequestContextHarness harness;
        prepareRequest(
            harness,
            "OPTIONS",
            "/api/auto-preflight",
            {
                {HttpHeaderNames::AccessControlRequestMethod, "GET"}
            });

        auto handler = config.handlerProviders().createContextHandler(harness.context());
        const auto capturedResponse = captureHandlerResponse(*handler, harness.context());

        TEST_ASSERT_EQUAL_UINT16(204, capturedResponse.status.code());

        const auto allowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowOrigin);
        const auto allowMethods = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowMethods);
        const auto allowHeaders = httpadv::v1::TestSupport::FindCapturedHeader(capturedResponse, HttpHeaderNames::AccessControlAllowHeaders);

        TEST_ASSERT_TRUE(allowOrigin.has_value());
        TEST_ASSERT_TRUE(allowMethods.has_value());
        TEST_ASSERT_TRUE(allowHeaders.has_value());
        TEST_ASSERT_EQUAL_STRING("https://api.example", allowOrigin->c_str());
        TEST_ASSERT_EQUAL_STRING("GET, OPTIONS", allowMethods->c_str());
        TEST_ASSERT_EQUAL_STRING("X-Test", allowHeaders->c_str());
    }

    void test_cors_handling_component_uri_pattern_filter_only_applies_to_matching_requests()
    {
        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);

        CorsHandlingOptions options;
        options.allowedOrigins = "https://api.example";
        options.allowedMethods = "GET, OPTIONS";
        options.allowedHeaders = "X-Test";
        config.use(CorsHandling("/api/**", options));

        HandlerMatcher apiMatcher("/api/ok", "GET");
        HandlerMatcher publicMatcher("/public/ok", "GET");

        config.add([apiMatcher](HttpRequestContext &context) mutable
        {
            return apiMatcher.canHandle(context);
        },
        [](HttpRequestContext &) -> HandlerResult
        {
            return HandlerResult::responseResult(createResponse(HttpStatus::Ok(), "api-ok"));
        });

        config.add([publicMatcher](HttpRequestContext &context) mutable
        {
            return publicMatcher.canHandle(context);
        },
        [](HttpRequestContext &) -> HandlerResult
        {
            return HandlerResult::responseResult(createResponse(HttpStatus::Ok(), "public-ok"));
        });

        RequestContextHarness apiHarness;
        prepareRequest(apiHarness, "GET", "/api/ok");
        auto apiHandler = config.handlerProviders().createContextHandler(apiHarness.context());
        const auto apiResponse = captureHandlerResponse(*apiHandler, apiHarness.context());

        const auto apiAllowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(apiResponse, HttpHeaderNames::AccessControlAllowOrigin);
        TEST_ASSERT_TRUE(apiAllowOrigin.has_value());
        TEST_ASSERT_EQUAL_STRING("https://api.example", apiAllowOrigin->c_str());

        RequestContextHarness publicHarness;
        prepareRequest(publicHarness, "GET", "/public/ok");
        auto publicHandler = config.handlerProviders().createContextHandler(publicHarness.context());
        const auto publicResponse = captureHandlerResponse(*publicHandler, publicHarness.context());

        const auto publicAllowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(publicResponse, HttpHeaderNames::AccessControlAllowOrigin);
        TEST_ASSERT_FALSE(publicAllowOrigin.has_value());
    }

    void test_cors_handling_component_predicate_filter_only_applies_when_predicate_matches()
    {
        HttpServerBase server(std::make_unique<httpadv::v1::TestSupport::FakeServer>());
        WebServerBuilder webBuilder(server);
        WebServerConfig config(server, webBuilder);

        CorsHandlingOptions options;
        options.allowedOrigins = "https://predicate.example";
        options.allowedMethods = "GET";
        options.allowedHeaders = "X-Test";

        config.use(CorsHandling(CorsRequestFilter([](HttpRequestContext &context)
        {
            return context.headers().exists("X-Enable-Cors", "true");
        }), options));

        HandlerMatcher matcher("/predicate", "GET");
        config.add([matcher](HttpRequestContext &context) mutable
        {
            return matcher.canHandle(context);
        },
        [](HttpRequestContext &) -> HandlerResult
        {
            return HandlerResult::responseResult(createResponse(HttpStatus::Ok(), "predicate-ok"));
        });

        RequestContextHarness enabledHarness;
        prepareRequest(enabledHarness, "GET", "/predicate", {{"X-Enable-Cors", "true"}});
        auto enabledHandler = config.handlerProviders().createContextHandler(enabledHarness.context());
        const auto enabledResponse = captureHandlerResponse(*enabledHandler, enabledHarness.context());

        const auto enabledAllowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(enabledResponse, HttpHeaderNames::AccessControlAllowOrigin);
        TEST_ASSERT_TRUE(enabledAllowOrigin.has_value());
        TEST_ASSERT_EQUAL_STRING("https://predicate.example", enabledAllowOrigin->c_str());

        RequestContextHarness disabledHarness;
        prepareRequest(disabledHarness, "GET", "/predicate");
        auto disabledHandler = config.handlerProviders().createContextHandler(disabledHarness.context());
        const auto disabledResponse = captureHandlerResponse(*disabledHandler, disabledHarness.context());

        const auto disabledAllowOrigin = httpadv::v1::TestSupport::FindCapturedHeader(disabledResponse, HttpHeaderNames::AccessControlAllowOrigin);
        TEST_ASSERT_FALSE(disabledAllowOrigin.has_value());
    }

    void test_replace_variables_streaming_replaces_tokens_across_source_chunks_and_normalizes_headers()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeaderNames::ContentType, "text/plain");
        headers.set(HttpHeaderNames::ContentLength, "30");

        std::unique_ptr<IHttpResponse> response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<httpadv::v1::TestSupport::ScriptedByteSource>(std::initializer_list<std::pair<const char *, bool>>{{"Hello {{na", false}, {"me}} from {{ci", false}, {"ty}}!", false}}),
            std::move(headers));

        const std::vector<std::pair<std::string, std::string>> values = {
            {"name", "Alice"},
            {"city", "Berlin"}};

        auto filter = ReplaceVariables(values);
        response = filter(std::move(response));

        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::ContentLength));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::TransferEncoding, "chunked"));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("Hello Alice from Berlin!", httpadv::v1::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_replace_variables_keeps_unresolved_tokens_by_default()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeaderNames::ContentType, "text/plain");

        std::unique_ptr<IHttpResponse> response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<httpadv::v1::TestSupport::ScriptedByteSource>(httpadv::v1::TestSupport::ScriptedByteSource::FromText("{{missing}} + {{known}}")),
            std::move(headers));

        const std::vector<std::pair<std::string, std::string>> values = {
            {"known", "ok"}};

        auto filter = ReplaceVariables(values);
        response = filter(std::move(response));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("{{missing}} + ok", httpadv::v1::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_replace_variables_respects_missing_value_policy_replace_with_empty()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeaderNames::ContentType, "text/plain");

        std::unique_ptr<IHttpResponse> response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<httpadv::v1::TestSupport::ScriptedByteSource>(httpadv::v1::TestSupport::ScriptedByteSource::FromText("{{missing}} + {{known}}")),
            std::move(headers));

        const std::vector<std::pair<std::string, std::string>> values = {
            {"known", "ok"}};

        ReplaceVariablesOptions options;
        options.missingValuePolicy = ReplaceVariablesMissingValuePolicy::ReplaceWithEmpty;
        auto filter = ReplaceVariables(values, options);
        response = filter(std::move(response));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING(" + ok", httpadv::v1::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_replace_variables_skips_non_text_content_types()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeaderNames::ContentType, "application/octet-stream");
        headers.set(HttpHeaderNames::ContentLength, "9");

        std::unique_ptr<IHttpResponse> response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<httpadv::v1::TestSupport::ScriptedByteSource>(httpadv::v1::TestSupport::ScriptedByteSource::FromText("bin {{x}}")),
            std::move(headers));

        const std::vector<std::pair<std::string, std::string>> values = {
            {"x", "ok"}};

        auto filter = ReplaceVariables(values);
        response = filter(std::move(response));

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentLength, "9"));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::TransferEncoding));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("bin {{x}}", httpadv::v1::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_replace_variables_skips_content_encoded_responses()
    {
        HttpHeaderCollection headers;
        headers.set(HttpHeaderNames::ContentType, "text/plain");
        headers.set(HttpHeaderNames::ContentEncoding, "gzip");
        headers.set(HttpHeaderNames::ContentLength, "8");

        std::unique_ptr<IHttpResponse> response = std::make_unique<HttpResponse>(
            HttpStatus::Ok(),
            std::make_unique<httpadv::v1::TestSupport::ScriptedByteSource>(httpadv::v1::TestSupport::ScriptedByteSource::FromText("{{x}}")),
            std::move(headers));

        const std::vector<std::pair<std::string, std::string>> values = {
            {"x", "ok"}};

        auto filter = ReplaceVariables(values);
        response = filter(std::move(response));

        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentEncoding, "gzip"));
        TEST_ASSERT_TRUE(response->headers().exists(HttpHeaderNames::ContentLength, "8"));
        TEST_ASSERT_FALSE(response->headers().exists(HttpHeaderNames::TransferEncoding));

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("{{x}}", httpadv::v1::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_handler_matcher_covers_exact_wildcard_and_parameter_extraction);
        RUN_TEST(test_handler_matcher_rejects_mismatched_methods_content_types_and_missing_metadata);
        RUN_TEST(test_handler_provider_registry_honors_beginning_and_end_order_and_custom_default_handler);
        RUN_TEST(test_handler_provider_registry_applies_filters_interceptors_and_body_forwarding_for_matches);
        RUN_TEST(test_handler_provider_registry_response_filters_only_run_for_non_null_responses);
        RUN_TEST(test_provider_registry_builder_on_factory_overload_registers_for_builder_and_web_server_config);
        RUN_TEST(test_provider_registry_builder_websocket_registration_creates_upgrade_handler);
        RUN_TEST(test_handler_provider_registry_supports_indexed_insertion_and_out_of_range_clamping);
        RUN_TEST(test_handler_provider_registry_uses_default_not_found_response_when_no_match_exists);
        RUN_TEST(test_handler_matcher_mutators_override_runtime_checker_and_extractor_behavior);
        RUN_TEST(test_default_uri_pattern_matching_uses_path_and_ignores_query_string_with_wildcards);
        RUN_TEST(test_handler_provider_registry_ignores_null_callbacks_in_composition_chain);
        RUN_TEST(test_handler_builder_composes_matchers_predicates_interceptors_and_filters);
        RUN_TEST(test_handler_registration_accepts_abstract_request_context_callbacks);
        RUN_TEST(test_basic_auth_rejects_missing_malformed_and_separatorless_credentials_with_challenge_header);
        RUN_TEST(test_basic_auth_accepts_valid_credentials_for_fixed_and_validator_overloads);
        RUN_TEST(test_cors_omits_optional_headers_and_repeated_application_preserves_existing_values);
        RUN_TEST(test_cors_does_not_overwrite_existing_headers);
        RUN_TEST(test_cors_handling_component_can_be_applied_through_web_server_config_use);
        RUN_TEST(test_cors_handling_component_auto_handles_preflight_options_without_manual_route);
        RUN_TEST(test_cors_handling_component_uri_pattern_filter_only_applies_to_matching_requests);
        RUN_TEST(test_cors_handling_component_predicate_filter_only_applies_when_predicate_matches);
        RUN_TEST(test_replace_variables_streaming_replaces_tokens_across_source_chunks_and_normalizes_headers);
        RUN_TEST(test_replace_variables_keeps_unresolved_tokens_by_default);
        RUN_TEST(test_replace_variables_respects_missing_value_policy_replace_with_empty);
        RUN_TEST(test_replace_variables_skips_non_text_content_types);
        RUN_TEST(test_replace_variables_skips_content_encoded_responses);
        return UNITY_END();
    }
}

int run_test_routing()
{
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "routing",
        runUnitySuite,
        localSetUp,
        localTearDown);
}


