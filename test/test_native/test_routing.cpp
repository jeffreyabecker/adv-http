#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/core/HttpHeader.h"
#include "../../src/core/HttpRequest.h"
#include "../../src/handlers/HandlerTypes.h"
#include "../../src/routing/BasicAuthentication.h"
#include "../../src/routing/CrossOriginRequestSharing.h"
#include "../../src/routing/HandlerMatcher.h"
#include "../../src/routing/HandlerProviderRegistry.h"
#include "../../src/routing/ProviderRegistryBuilder.h"
#include "../../src/response/StringResponse.h"
#include "../../src/server/HttpServerBase.h"

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

using namespace HttpServerAdvanced;

namespace
{
    std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string_view body, std::initializer_list<HttpHeader> headers = {});

    class NoOpHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpRequest &) override
        {
            return nullptr;
        }

        void handleBodyChunk(HttpRequest &, const uint8_t *, std::size_t) override
        {
        }
    };

    class RecordingHandler : public IHttpHandler
    {
    public:
        using StepCallback = std::function<HandlerResult(HttpRequest &)>;

        explicit RecordingHandler(StepCallback stepCallback = nullptr)
            : stepCallback_(std::move(stepCallback))
        {
        }

        HandlerResult handleStep(HttpRequest &context) override
        {
            ++handleStepCount_;
            if (stepCallback_)
            {
                return stepCallback_(context);
            }
            return nullptr;
        }

        void handleBodyChunk(HttpRequest &, const uint8_t *at, std::size_t length) override
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
        explicit StaticResponseProvider(std::string responseBody, IHttpHandler::Predicate predicate = nullptr)
            : responseBody_(std::move(responseBody)),
              predicate_(std::move(predicate))
        {
        }

        bool canHandle(HttpRequest &context) override
        {
            ++canHandleCount_;
            return predicate_ ? predicate_(context) : true;
        }

        std::unique_ptr<IHttpHandler> create(HttpRequest &) override
        {
            ++createCount_;
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Ok(), responseBody_),
                [](const HttpRequest &)
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
        IHttpHandler::Predicate predicate_;
        std::size_t canHandleCount_ = 0;
        std::size_t createCount_ = 0;
    };

    class RequestContextHarness
    {
    public:
        RequestContextHarness()
            : server_(std::make_unique<HttpServerBase>(std::make_unique<TestSupport::FakeServer>())),
              handler_(std::make_unique<NoOpHandler>()),
              factory_([this](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
              {
                  context_ = &context;
                  return std::move(handler_);
              }),
              pipeline_(HttpRequest::createPipelineHandler(*server_, factory_))
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

        HttpRequest &context()
        {
            TEST_ASSERT_NOT_NULL(context_);
            return *context_;
        }

    private:
        std::unique_ptr<HttpServerBase> server_;
        std::unique_ptr<IHttpHandler> handler_;
        TestSupport::RecordingRequestHandlerFactory factory_;
        PipelineHandlerPtr pipeline_;
        HttpRequest *context_ = nullptr;
        std::string methodStorage_;
        std::string urlStorage_;
        std::string nameStorage_;
        std::string valueStorage_;
    };

    struct AuthInvocationResult
    {
        bool nextCalled = false;
        std::optional<TestSupport::CapturedResponse> response;
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

    TestSupport::CapturedResponse captureHandlerResponse(IHttpHandler &handler, HttpRequest &context)
    {
        return TestSupport::CaptureResponse(handler.handleStep(context));
    }

    AuthInvocationResult invokeAuth(IHttpHandler::InterceptorCallback interceptor, HttpRequest &context)
    {
        AuthInvocationResult result;
        auto response = interceptor(context, [&result](HttpRequest &) -> std::unique_ptr<IHttpResponse>
        {
            result.nextCalled = true;
            return createResponse(HttpStatus::Ok(), "authorized");
        });

        if (response)
        {
            result.response = TestSupport::CaptureResponse(std::move(response));
        }

        return result;
    }

    void test_handler_matcher_covers_exact_wildcard_and_parameter_extraction()
    {
        const std::string filesWildcardPattern = std::string("/files/") + REQUEST_MATCHER_PATH_WILDCARD_CHAR;
        const std::string nestedWildcardPattern = std::string("/users/") + REQUEST_MATCHER_PATH_WILDCARD_CHAR + "/photos/" + REQUEST_MATCHER_PATH_WILDCARD_CHAR;

        RequestContextHarness exactHarness;
        prepareRequest(
            exactHarness,
            "GET",
            "/files/report.txt",
            {{"Content-Type", "Application/Json; charset=utf-8"}});
        HttpRequest &exactContext = exactHarness.context();

        TEST_ASSERT_TRUE(defaultCheckUriPattern("/files/report.txt", "/files/report.txt"));
        TEST_ASSERT_TRUE(defaultCheckUriPattern("/files/report.txt", filesWildcardPattern));
        TEST_ASSERT_TRUE(defaultCheckMethod("GET,POST", "GET"));
        TEST_ASSERT_TRUE(defaultCheckContentType(exactContext, {"application/json"}));

        HandlerMatcher wildcardMatcher(filesWildcardPattern, "GET");
        TEST_ASSERT_TRUE(wildcardMatcher.canHandle(exactContext));

        const RouteParameters exactParams = wildcardMatcher.extractParameters(exactContext);
        TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(exactParams.size()));
        TEST_ASSERT_EQUAL_STRING("report.txt", exactParams[0].c_str());

        RequestContextHarness nestedHarness;
        prepareRequest(nestedHarness, "GET", "/users/42/photos/cover");
        HandlerMatcher nestedMatcher(nestedWildcardPattern, "GET");
        const RouteParameters nestedParams = nestedMatcher.extractParameters(nestedHarness.context());
        TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(nestedParams.size()));
        TEST_ASSERT_EQUAL_STRING("42", nestedParams[0].c_str());
        TEST_ASSERT_EQUAL_STRING("cover", nestedParams[1].c_str());
    }

    void test_handler_matcher_rejects_mismatched_methods_content_types_and_missing_metadata()
    {
        const std::string filesWildcardPattern = std::string("/files/") + REQUEST_MATCHER_PATH_WILDCARD_CHAR;
        const std::string assetsWildcardPattern = std::string("/assets/") + REQUEST_MATCHER_PATH_WILDCARD_CHAR;

        RequestContextHarness jsonHarness;
        prepareRequest(
            jsonHarness,
            "POST",
            "/files/report.txt",
            {{"Content-Type", "text/plain"}});
        HttpRequest &jsonContext = jsonHarness.context();

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
    }

    void test_handler_provider_registry_honors_beginning_and_end_order_and_custom_default_handler()
    {
        RequestContextHarness harness;
        prepareRequest(harness, "GET", "/registry/order");
        HttpRequest &context = harness.context();

        StaticResponseProvider beginningProvider("beginning");
        StaticResponseProvider firstEndProvider("first-end", [](HttpRequest &request)
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
        StaticResponseProvider nonMatchingProvider("never", [](HttpRequest &request)
        {
            return request.headers().exists("X-End-Route", "missing");
        });
        StaticResponseProvider matchingEndProvider("matched-end", [](HttpRequest &request)
        {
            return request.headers().exists("X-End-Route", "first");
        });
        endRegistry.add(nonMatchingProvider, HandlerProviderRegistry::AddAt::End);
        endRegistry.add(matchingEndProvider, HandlerProviderRegistry::AddAt::End);
        endRegistry.setDefaultHandlerFactory([](HttpRequest &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::NotFound(), "custom-default"),
                [](const HttpRequest &)
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
        HttpRequest &context = harness.context();

        HandlerProviderRegistry registry;
        std::vector<std::string> trace;
        RecordingHandler *innerHandler = nullptr;

        registry.filterRequest([](HttpRequest &request)
        {
            return request.methodView() == "POST";
        });
        registry.filterRequest([](HttpRequest &request)
        {
            return request.headers().exists("X-Allow", "yes");
        });

        registry.with([&trace](HttpRequest &request, IHttpHandler::InvocationCallback next) -> std::unique_ptr<IHttpResponse>
        {
            trace.push_back("interceptor-1-before");
            auto response = next(request);
            trace.push_back("interceptor-1-after");
            return response;
        });
        registry.with([&trace](HttpRequest &request, IHttpHandler::InvocationCallback next) -> std::unique_ptr<IHttpResponse>
        {
            trace.push_back("interceptor-2-before");
            auto response = next(request);
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
            [](HttpRequest &)
            {
                return true;
            },
            [&innerHandler, &trace](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                auto handler = std::make_unique<RecordingHandler>([&trace](HttpRequest &) -> std::unique_ptr<IHttpResponse>
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

        const auto filterHeader = TestSupport::FindCapturedHeader(response, "X-Filter");
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
        registry.setDefaultHandlerFactory([](HttpRequest &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::Forbidden(), "blocked"),
                [](const HttpRequest &)
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
        HttpRequest &context = harness.context();

        HandlerProviderRegistry registry;
        int filterCallCount = 0;
        registry.apply([&filterCallCount](std::unique_ptr<IHttpResponse> response) -> std::unique_ptr<IHttpResponse>
        {
            ++filterCallCount;
            return response;
        });

        registry.add(
            [](HttpRequest &)
            {
                return true;
            },
            [](HttpRequest &) -> std::unique_ptr<IHttpHandler>
            {
                return std::make_unique<RecordingHandler>();
            });

        auto handler = registry.createContextHandler(context);
        const auto response = handler->handleStep(context);
        TEST_ASSERT_NULL(response.get());
        TEST_ASSERT_EQUAL_INT(0, filterCallCount);
    }

    void test_handler_builder_composes_matchers_predicates_interceptors_and_filters()
    {
        HandlerProviderRegistry registry;
        registry.setDefaultHandlerFactory([](HttpRequest &) -> std::unique_ptr<IHttpHandler>
        {
            return std::make_unique<HttpHandler>(
                createResponse(HttpStatus::NotFound(), "builder-default"),
                [](const HttpRequest &)
                {
                    return true;
                });
        });

        ProviderRegistryBuilder builder(registry);
        {
            HandlerMatcher matcher(
                "/docs/*",
                defaultCheckMethod,
                [](std::string_view, std::string_view)
                {
                    return true;
                },
                defaultCheckContentType,
                [](HttpRequest &, std::string_view)
                {
                    return RouteParameters{"readme"};
                },
                "get",
                {"Application/Json"});

            auto route = builder.on<GetRequest>(matcher, [](HttpRequest &request, RouteParameters &&params) -> std::unique_ptr<IHttpResponse>
            {
                TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(params.size()));
                request.items()["route-param"] = params[0];
                return createResponse(HttpStatus::Ok(), std::string("doc:") + params[0]);
            });

            route.filterRequest([](HttpRequest &request)
                {
                    return request.headers().exists("X-Route", "yes");
                })
                .filterRequest([](HttpRequest &request)
                {
                    return !request.headers().exists("X-Block", "true");
                })
                .allowMethods("get")
                .allowContentTypes({"Application/Json"})
                .with([](HttpRequest &request, IHttpHandler::InvocationCallback next) -> std::unique_ptr<IHttpResponse>
                {
                    request.items()["builder-interceptor"] = true;
                    return next(request);
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
        HttpRequest &matchedContext = matchedHarness.context();

        auto matchedHandler = registry.createContextHandler(matchedContext);
        const auto matchedResponse = captureHandlerResponse(*matchedHandler, matchedContext);
        TEST_ASSERT_EQUAL_STRING("doc:readme", matchedResponse.body.c_str());
        TEST_ASSERT_TRUE(std::any_cast<bool>(matchedContext.items().at("builder-interceptor")));

        const auto routeParam = std::any_cast<std::string>(&matchedContext.items().at("route-param"));
        TEST_ASSERT_NOT_NULL(routeParam);
        TEST_ASSERT_EQUAL_STRING("readme", routeParam->c_str());

        const auto builderFilter = TestSupport::FindCapturedHeader(matchedResponse, "X-Builder-Filter");
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

    void test_basic_auth_rejects_missing_malformed_and_separatorless_credentials_with_challenge_header()
    {
        auto auth = BasicAuth("admin", "secret", "Admin Area");

        RequestContextHarness missingHeaderHarness;
        prepareRequest(missingHeaderHarness, "GET", "/admin");
        const auto missingHeaderResult = invokeAuth(auth, missingHeaderHarness.context());
        TEST_ASSERT_FALSE(missingHeaderResult.nextCalled);
        TEST_ASSERT_TRUE(missingHeaderResult.response.has_value());
        TEST_ASSERT_EQUAL_UINT16(401, missingHeaderResult.response->status.code());
        const auto missingHeaderChallenge = TestSupport::FindCapturedHeader(*missingHeaderResult.response, HttpHeaderNames::WwwAuthenticate);
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

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_handler_matcher_covers_exact_wildcard_and_parameter_extraction);
        RUN_TEST(test_handler_matcher_rejects_mismatched_methods_content_types_and_missing_metadata);
        RUN_TEST(test_handler_provider_registry_honors_beginning_and_end_order_and_custom_default_handler);
        RUN_TEST(test_handler_provider_registry_applies_filters_interceptors_and_body_forwarding_for_matches);
        RUN_TEST(test_handler_provider_registry_response_filters_only_run_for_non_null_responses);
        RUN_TEST(test_handler_builder_composes_matchers_predicates_interceptors_and_filters);
        RUN_TEST(test_basic_auth_rejects_missing_malformed_and_separatorless_credentials_with_challenge_header);
        RUN_TEST(test_basic_auth_accepts_valid_credentials_for_fixed_and_validator_overloads);
        RUN_TEST(test_cors_omits_optional_headers_and_repeated_application_preserves_existing_values);
        RUN_TEST(test_cors_does_not_overwrite_existing_headers);
        return UNITY_END();
    }
}

int run_test_routing()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "routing",
        runUnitySuite,
        localSetUp,
        localTearDown);
}