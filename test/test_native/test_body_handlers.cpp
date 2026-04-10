#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include "../../src/lumalink/http/core/HttpContext.h"
#include "../../src/lumalink/http/core/HttpContextPhase.h"
#include "../../src/lumalink/http/handlers/BufferedStringBodyHandler.h"
#include "../../src/lumalink/http/handlers/BufferingHttpHandlerBase.h"
#include "../../src/lumalink/http/handlers/FormBodyHandler.h"
#include "../../src/lumalink/http/handlers/MultipartFormDataHandler.h"
#include "../../src/lumalink/http/handlers/RawBodyHandler.h"
#include "../../src/lumalink/http/server/HttpServerBase.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include <ArduinoJson.h>
#include "../../src/lumalink/http/handlers/JsonBodyHandler.h"
#endif

using namespace lumalink::http::core;
using namespace lumalink::http::handlers;
using namespace lumalink::http::pipeline;
using namespace lumalink::http::response;
using namespace lumalink::http::routing;
using namespace lumalink::http::server;
using namespace lumalink::http::staticfiles;
using namespace lumalink::platform::buffers;
using namespace lumalink::platform::transport;
using namespace lumalink::platform::buffers;
using namespace lumalink::http::util;
using namespace lumalink::http::websocket;

namespace
{
    #include <cstddef>
    template <std::ptrdiff_t MaxBuffered>
    class BufferingProbeHandler : public BufferingHttpHandlerBase<MaxBuffered>
    {
    public:
        typename IHttpHandler::HandlerResult handleBody(HttpRequestContext &context, std::vector<uint8_t> &&body) override
        {
            phases.push_back(context.completedPhases());
            payloads.emplace_back(body.begin(), body.end());
            return nullptr;
        }

        std::vector<HttpContextPhaseFlags> phases;
        std::vector<std::vector<uint8_t>> payloads;
    };

    class RequestHarness
    {
    public:
        explicit RequestHarness(std::unique_ptr<IHttpHandler> handler)
            : server_(std::make_unique<lumalink::http::TestSupport::FakeServer>()),
              pendingHandler_(std::move(handler)),
              handlerPtr_(pendingHandler_.get()),
              factory_([this](HttpRequestContext &) -> std::unique_ptr<IHttpHandler>
              {
                  return std::move(pendingHandler_);
              }),
              pipeline_(HttpContext::createPipelineHandler(server_, factory_))
        {
        }

        template <typename HandlerT>
        HandlerT *handlerAs() const
        {
            return static_cast<HandlerT *>(handlerPtr_);
        }

        void beginRequest(const char *method = "POST", const char *url = "/body")
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageBegin(method, 1, 1, url));
        }

        void addHeader(std::string_view name, std::string_view value)
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeader(name, value));
        }

        void completeHeaders()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onHeadersComplete());
            capturePendingResult();
        }

        void addBody(std::string_view text)
        {
            TEST_ASSERT_EQUAL_INT(
                0,
                pipeline_->onBody(
                    reinterpret_cast<const std::uint8_t *>(text.data()),
                    text.size()));
            capturePendingResult();
        }

        void completeMessage()
        {
            TEST_ASSERT_EQUAL_INT(0, pipeline_->onMessageComplete());
            capturePendingResult();
        }

        std::size_t responseStreamCount() const
        {
            return responseStreamCount_;
        }

        const std::string &responseText() const
        {
            return responseText_;
        }

    private:
        void capturePendingResult()
        {
            if (!pipeline_->hasPendingResult())
            {
                return;
            }

            RequestHandlingResult result = pipeline_->takeResult();
            if (result.has_value() && result->kind == RequestHandlingSuccess::Kind::Response && result->responseStream)
            {
                ++responseStreamCount_;
                responseText_ = lumalink::http::TestSupport::ReadByteSourceAsStdString(*result->responseStream);
            }
        }

        HttpServerBase server_;
        std::unique_ptr<IHttpHandler> pendingHandler_;
        IHttpHandler *handlerPtr_ = nullptr;
        lumalink::http::TestSupport::RecordingRequestHandlerFactory factory_;
        std::unique_ptr<IPipelineHandler, std::function<void(IPipelineHandler *)>> pipeline_;
        std::size_t responseStreamCount_ = 0;
        std::string responseText_;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_try_parse_header_length_accepts_complete_numeric_values()
    {
        size_t parsedLength = 999;

        TEST_ASSERT_TRUE(tryParseHeaderLength("0", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_TRUE(tryParseHeaderLength("12345", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(12345, parsedLength);
    }

    void test_try_parse_header_length_rejects_invalid_or_partial_values()
    {
        size_t parsedLength = 77;

        TEST_ASSERT_FALSE(tryParseHeaderLength("", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength("12x", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(12, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength(" 12", parsedLength));
        TEST_ASSERT_EQUAL_UINT64(0, parsedLength);

        TEST_ASSERT_FALSE(tryParseHeaderLength("18446744073709551616", parsedLength));
    }

    void test_buffering_handler_does_not_buffer_when_disabled()
    {
        auto handler = std::make_unique<BufferingProbeHandler<0>>();
        BufferingProbeHandler<0> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "4");
        harness.completeHeaders();
        harness.addBody("test");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(0, harness.responseStreamCount());
    }

    void test_buffering_handler_reinvokes_handle_body_on_completion_after_content_length_is_reached()
    {
        auto handler = std::make_unique<BufferingProbeHandler<8>>();
        BufferingProbeHandler<8> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "4");
        harness.completeHeaders();
        harness.addBody("test");

        TEST_ASSERT_EQUAL_UINT64(1, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("test"), handlerPtr->payloads[0].data(), 4);
        TEST_ASSERT_TRUE((handlerPtr->phases[0] & HttpContextPhase::BeginReadingBody) != 0);
        TEST_ASSERT_FALSE((handlerPtr->phases[0] & HttpContextPhase::CompletedReadingMessage) != 0);

        harness.completeMessage();
        TEST_ASSERT_EQUAL_UINT64(2, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[1].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("test"), handlerPtr->payloads[1].data(), 4);
        TEST_ASSERT_TRUE((handlerPtr->phases[1] & HttpContextPhase::CompletedReadingMessage) != 0);
    }

    void test_buffering_handler_invokes_handle_body_from_handle_step_without_content_length()
    {
        auto handler = std::make_unique<BufferingProbeHandler<-1>>();
        BufferingProbeHandler<-1> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody("abc");
        harness.addBody("def");

        TEST_ASSERT_EQUAL_UINT64(0, handlerPtr->payloads.size());

        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(6, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcdef"), handlerPtr->payloads[0].data(), 6);
        TEST_ASSERT_TRUE((handlerPtr->phases[0] & HttpContextPhase::CompletedReadingMessage) != 0);
    }

    void test_buffering_handler_truncates_to_configured_max_buffer_and_replays_on_completion()
    {
        auto handler = std::make_unique<BufferingProbeHandler<4>>();
        BufferingProbeHandler<4> *handlerPtr = handler.get();
        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "8");
        harness.completeHeaders();
        harness.addBody("abcdef");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(2, handlerPtr->payloads.size());
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcd"), handlerPtr->payloads[0].data(), 4);
        TEST_ASSERT_EQUAL_UINT64(4, handlerPtr->payloads[1].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("abcd"), handlerPtr->payloads[1].data(), 4);
    }

    void test_buffering_handler_ignores_empty_body_and_zero_content_length()
    {
        auto missingLengthHandler = std::make_unique<BufferingProbeHandler<16>>();
        BufferingProbeHandler<16> *missingLengthPtr = missingLengthHandler.get();
        RequestHarness missingLengthHarness(std::move(missingLengthHandler));

        missingLengthHarness.beginRequest();
        missingLengthHarness.completeHeaders();
        missingLengthHarness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, missingLengthPtr->payloads.size());

        auto zeroLengthHandler = std::make_unique<BufferingProbeHandler<16>>();
        BufferingProbeHandler<16> *zeroLengthPtr = zeroLengthHandler.get();
        RequestHarness zeroLengthHarness(std::move(zeroLengthHandler));

        zeroLengthHarness.beginRequest();
        zeroLengthHarness.addHeader(HttpHeaderNames::ContentLength, "0");
        zeroLengthHarness.completeHeaders();
        zeroLengthHarness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, zeroLengthPtr->payloads.size());
    }

    void test_buffered_string_body_handler_preserves_exact_bytes_in_std_string()
    {
        std::vector<std::string> capturedBodies;
        std::vector<RouteParameters> capturedParams;

        auto handler = std::make_unique<BufferedStringBodyHandler>(
            [&capturedBodies, &capturedParams](HttpRequestContext &, RouteParameters &&params, std::string &&body) -> IHttpHandler::HandlerResult
            {
                capturedParams.push_back(std::move(params));
                capturedBodies.push_back(std::move(body));
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {{"route-id", "42"}};
            });

        RequestHarness harness(std::move(handler));
        const std::string payload("A\0B", 3);

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody(std::string_view(payload.data(), payload.size()));
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, capturedBodies.size());
        TEST_ASSERT_EQUAL_UINT64(3, capturedBodies[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>(payload.data()), reinterpret_cast<const uint8_t *>(capturedBodies[0].data()), payload.size());
        TEST_ASSERT_EQUAL_UINT64(1, capturedParams[0].size());
        TEST_ASSERT_EQUAL_STRING("42", capturedParams[0].at("route-id").c_str());
    }

    void test_buffered_string_body_handler_ignores_empty_payloads()
    {
        std::size_t invocationCount = 0;

        auto handler = std::make_unique<BufferedStringBodyHandler>(
            [&invocationCount](HttpRequestContext &, RouteParameters &&, std::string &&) -> IHttpHandler::HandlerResult
            {
                ++invocationCount;
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "0");
        harness.completeHeaders();
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, invocationCount);
    }

    void test_raw_body_handler_preserves_chunks_and_completion_metadata()
    {
        std::vector<RawBodyBuffer> capturedBuffers;
        std::vector<RouteParameters> capturedParams;

        auto handler = std::make_unique<RawBodyHandler>(
            [&capturedBuffers, &capturedParams](HttpRequestContext &, RouteParameters &params, RawBodyBuffer buffer) -> IHttpHandler::HandlerResult
            {
                capturedParams.push_back(params);
                capturedBuffers.push_back(std::move(buffer));
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {{"buffer", "alpha"}};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "5");
        harness.completeHeaders();
        harness.addBody("ab");
        harness.addBody("cde");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(3, capturedBuffers.size());
        TEST_ASSERT_EQUAL_UINT64(2, capturedBuffers[0].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("ab"), capturedBuffers[0].data(), 2);
        TEST_ASSERT_EQUAL_UINT64(0, capturedBuffers[0].receivedLength());
        TEST_ASSERT_EQUAL_UINT64(5, capturedBuffers[0].contentLength());

        TEST_ASSERT_EQUAL_UINT64(3, capturedBuffers[1].size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>("cde"), capturedBuffers[1].data(), 3);
        TEST_ASSERT_EQUAL_UINT64(2, capturedBuffers[1].receivedLength());
        TEST_ASSERT_EQUAL_UINT64(5, capturedBuffers[1].contentLength());

        TEST_ASSERT_EQUAL_UINT64(0, capturedBuffers[2].size());
        TEST_ASSERT_EQUAL_UINT64(5, capturedBuffers[2].receivedLength());
        TEST_ASSERT_EQUAL_UINT64(5, capturedBuffers[2].contentLength());
        TEST_ASSERT_EQUAL_UINT64(1, capturedParams[0].size());
        TEST_ASSERT_EQUAL_STRING("alpha", capturedParams[0].at("buffer").c_str());
    }

    void test_raw_body_handler_passes_large_chunks_without_internal_truncation()
    {
        std::vector<RawBodyBuffer> capturedBuffers;
        auto handler = std::make_unique<RawBodyHandler>(
            [&capturedBuffers](HttpRequestContext &, RouteParameters &, RawBodyBuffer buffer) -> IHttpHandler::HandlerResult
            {
                capturedBuffers.push_back(std::move(buffer));
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));
        const std::string payload(3000, 'x');

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "3000");
        harness.completeHeaders();
        harness.addBody(payload);
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(2, capturedBuffers.size());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), capturedBuffers[0].size());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), capturedBuffers[0].contentLength());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t *>(payload.data()), capturedBuffers[0].data(), payload.size());
        TEST_ASSERT_EQUAL_UINT64(0, capturedBuffers[1].size());
        TEST_ASSERT_EQUAL_UINT64(payload.size(), capturedBuffers[1].receivedLength());
    }

    void test_form_body_handler_parses_repeated_empty_and_malformed_values()
    {
        std::vector<WebUtility::QueryParameters> capturedBodies;
        std::vector<RouteParameters> capturedParams;

        auto handler = std::make_unique<FormBodyHandler>(
            [&capturedBodies, &capturedParams](HttpRequestContext &, RouteParameters &&params, WebUtility::QueryParameters &&body) -> IHttpHandler::HandlerResult
            {
                capturedParams.push_back(std::move(params));
                capturedBodies.push_back(std::move(body));
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {{"resource", "route"}, {"kind", "form"}};
            });

        RequestHarness harness(std::move(handler));
        const std::string_view body = "=value&&name=&name=second&bad=%ZZ&trailing=";

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody(body);
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, capturedBodies.size());
        TEST_ASSERT_EQUAL_UINT64(6, capturedBodies[0].pairs().size());
        TEST_ASSERT_EQUAL_UINT64(2, capturedBodies[0].exists("name"));
        TEST_ASSERT_EQUAL_UINT64(2, capturedBodies[0].exists(""));

        const auto emptyKey = capturedBodies[0].get("");
        TEST_ASSERT_TRUE(emptyKey.has_value());
        TEST_ASSERT_EQUAL_STRING("value", emptyKey->c_str());

        const auto allNames = capturedBodies[0].getAll("name");
        TEST_ASSERT_EQUAL_UINT64(2, allNames.size());
        TEST_ASSERT_EQUAL_STRING("", allNames[0].c_str());
        TEST_ASSERT_EQUAL_STRING("second", allNames[1].c_str());

        const auto bad = capturedBodies[0].get("bad");
        TEST_ASSERT_TRUE(bad.has_value());
        TEST_ASSERT_EQUAL_UINT64(1, bad->size());
        TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>((*bad)[0]));

        const auto trailing = capturedBodies[0].get("trailing");
        TEST_ASSERT_TRUE(trailing.has_value());
        TEST_ASSERT_TRUE(trailing->empty());

        TEST_ASSERT_EQUAL_UINT64(2, capturedParams[0].size());
        TEST_ASSERT_EQUAL_STRING("route", capturedParams[0].at("resource").c_str());
        TEST_ASSERT_EQUAL_STRING("form", capturedParams[0].at("kind").c_str());
    }

    void test_form_body_handler_ignores_empty_payloads()
    {
        std::size_t invocationCount = 0;

        auto handler = std::make_unique<FormBodyHandler>(
            [&invocationCount](HttpRequestContext &, RouteParameters &&, WebUtility::QueryParameters &&) -> IHttpHandler::HandlerResult
            {
                ++invocationCount;
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, invocationCount);
    }

    void test_multipart_handler_requires_boundary_header()
    {
        auto handler = std::make_unique<MultipartFormDataHandler>(
            [](HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer) -> IHttpHandler::HandlerResult
            {
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentType, HttpContentTypes::MultipartFormData);
        harness.completeHeaders();
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, harness.responseStreamCount());
        TEST_ASSERT_NOT_EQUAL(std::string::npos, harness.responseText().find("HTTP/1.1 400 Bad Request"));
        TEST_ASSERT_NOT_EQUAL(std::string::npos, harness.responseText().find("Missing or invalid boundary"));
    }

    void test_multipart_handler_parses_named_part_and_emits_completed_event()
    {
        struct MultipartEvent
        {
            MultipartStatus status = MultipartStatus::Completed;
            std::string name;
            std::string filename;
            std::string contentType;
            std::string data;
        };

        std::vector<MultipartEvent> capturedParts;
        auto handler = std::make_unique<MultipartFormDataHandler>(
            [&capturedParts](HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer buffer) -> IHttpHandler::HandlerResult
            {
                capturedParts.push_back({
                    buffer.status(),
                    std::string(buffer.name()),
                    std::string(buffer.filename()),
                    std::string(buffer.contentType()),
                    std::string(reinterpret_cast<const char *>(buffer.data()), buffer.size())});
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                    return {{"part", "route-part"}};
            });

        RequestHarness harness(std::move(handler));
        const std::string body =
            "Content-Disposition: form-data; name=\"field\"; filename=\"note.txt\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "hello\r\n--b\r\n";

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentType, "multipart/form-data; boundary=--b");
        harness.completeHeaders();
        harness.addBody(body);
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(2, capturedParts.size());
        TEST_ASSERT_EQUAL(MultipartStatus::FirstChunk, capturedParts[0].status);
        TEST_ASSERT_EQUAL_STRING("hello", capturedParts[0].data.c_str());
        TEST_ASSERT_EQUAL_STRING("field", capturedParts[0].name.c_str());
        TEST_ASSERT_EQUAL_STRING("note.txt", capturedParts[0].filename.c_str());
        TEST_ASSERT_EQUAL_STRING("text/plain", capturedParts[0].contentType.c_str());

        TEST_ASSERT_EQUAL(MultipartStatus::Completed, capturedParts[1].status);
        TEST_ASSERT_TRUE(capturedParts[1].data.empty());
        TEST_ASSERT_TRUE(capturedParts[1].name.empty());
    }

    void test_multipart_handler_skips_empty_parts_and_only_emits_completed_event()
    {
        struct MultipartEvent
        {
            MultipartStatus status = MultipartStatus::Completed;
            std::string name;
            std::size_t size = 0;
        };

        std::vector<MultipartEvent> capturedParts;
        auto handler = std::make_unique<MultipartFormDataHandler>(
            [&capturedParts](HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer buffer) -> IHttpHandler::HandlerResult
            {
                capturedParts.push_back({buffer.status(), std::string(buffer.name()), buffer.size()});
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));
        const std::string body =
            "Content-Disposition: form-data; name=\"empty\"\r\n\r\n"
            "\r\n--b\r\n";

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentType, "multipart/form-data; boundary=--b");
        harness.completeHeaders();
        harness.addBody(body);
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(2, capturedParts.size());
        TEST_ASSERT_EQUAL(MultipartStatus::FirstChunk, capturedParts[0].status);
        TEST_ASSERT_EQUAL_STRING("empty", capturedParts[0].name.c_str());
        TEST_ASSERT_EQUAL_UINT64(0, capturedParts[0].size);
        TEST_ASSERT_EQUAL(MultipartStatus::Completed, capturedParts[1].status);
    }

    void test_multipart_handler_drops_small_truncated_part_payload_before_completed_event()
    {
        std::vector<MultipartFormDataBuffer> capturedParts;
        auto handler = std::make_unique<MultipartFormDataHandler>(
            [&capturedParts](HttpRequestContext &, RouteParameters &, MultipartFormDataBuffer buffer) -> IHttpHandler::HandlerResult
            {
                capturedParts.push_back(std::move(buffer));
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));
        const std::string body =
            "Content-Disposition: form-data; name=\"field\"\r\n\r\n"
            "hello";

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentType, "multipart/form-data; boundary=--b");
        harness.completeHeaders();
        harness.addBody(body);
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, capturedParts.size());
        TEST_ASSERT_EQUAL(MultipartStatus::Completed, capturedParts[0].status());
        TEST_ASSERT_EQUAL_UINT64(0, capturedParts[0].size());
    }

#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
    void test_json_body_handler_parses_valid_json_and_preserves_route_parameters()
    {
        std::vector<RouteParameters> capturedParams;
        std::vector<std::string> capturedMessages;
        std::vector<int> capturedCounts;
        std::vector<bool> sawParseErrors;

        auto handler = std::make_unique<JsonBodyHandler>(
            [&capturedParams, &capturedMessages, &capturedCounts, &sawParseErrors](HttpRequestContext &context, RouteParameters &&params, JsonDocument &&body) -> IHttpHandler::HandlerResult
            {
                capturedParams.push_back(std::move(params));
                capturedMessages.emplace_back(body["message"].template as<std::string>());
                capturedCounts.push_back(body["count"].template as<int>());
                sawParseErrors.push_back(Json::deserializationError(context) != nullptr);
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {{"format", "json"}, {"scope", "route"}};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody("{\"message\":\"ok\",\"count\":2}");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, capturedMessages.size());
        TEST_ASSERT_EQUAL_STRING("ok", capturedMessages[0].c_str());
        TEST_ASSERT_EQUAL(2, capturedCounts[0]);
        TEST_ASSERT_EQUAL_UINT64(2, capturedParams[0].size());
        TEST_ASSERT_EQUAL_STRING("json", capturedParams[0].at("format").c_str());
        TEST_ASSERT_EQUAL_STRING("route", capturedParams[0].at("scope").c_str());
        TEST_ASSERT_EQUAL_UINT64(1, sawParseErrors.size());
        TEST_ASSERT_FALSE(sawParseErrors[0]);
    }

    void test_json_body_handler_passes_empty_documents_for_malformed_payloads()
    {
        std::vector<bool> messagePresent;
        std::vector<bool> sawParseErrors;

        auto handler = std::make_unique<JsonBodyHandler>(
            [&messagePresent, &sawParseErrors](HttpRequestContext &context, RouteParameters &&, JsonDocument &&body) -> IHttpHandler::HandlerResult
            {
                messagePresent.push_back(!body["message"].isNull());
                sawParseErrors.push_back(Json::deserializationError(context) != nullptr);
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody("{\"message\":");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, messagePresent.size());
        TEST_ASSERT_FALSE(messagePresent[0]);
        TEST_ASSERT_EQUAL_UINT64(1, sawParseErrors.size());
        TEST_ASSERT_TRUE(sawParseErrors[0]);
    }

    void test_json_body_handler_ignores_empty_payloads()
    {
        std::size_t invocationCount = 0;

        auto handler = std::make_unique<JsonBodyHandler>(
            [&invocationCount](HttpRequestContext &, RouteParameters &&, JsonDocument &&) -> IHttpHandler::HandlerResult
            {
                ++invocationCount;
                return nullptr;
            },
            [](HttpRequestContext &) -> RouteParameters
            {
                return {};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader(HttpHeaderNames::ContentLength, "0");
        harness.completeHeaders();
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(0, invocationCount);
    }
#else
    void test_json_body_handler_is_unavailable_in_json_disabled_build()
    {
        TEST_ASSERT_EQUAL(0, LUMALINK_HTTP_ENABLE_ARDUINO_JSON);
    }
#endif

    void test_raw_body_handler_extractor_runs_on_first_body_chunk_phase()
    {
        std::vector<HttpContextPhaseFlags> extractorPhases;

        auto handler = std::make_unique<RawBodyHandler>(
            [](HttpRequestContext &, RouteParameters &, RawBodyBuffer) -> IHttpHandler::HandlerResult
            {
                return nullptr;
            },
            [&extractorPhases](HttpRequestContext &context) -> RouteParameters
            {
                extractorPhases.push_back(context.completedPhases());
                return {{"kind", "raw"}};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.completeHeaders();
        harness.addBody("x");
        harness.completeMessage();

        TEST_ASSERT_EQUAL_UINT64(1, extractorPhases.size());
        TEST_ASSERT_TRUE((extractorPhases[0] & HttpContextPhase::BeginReadingBody) != 0);
        TEST_ASSERT_FALSE((extractorPhases[0] & HttpContextPhase::CompletedReadingMessage) != 0);
    }

    void test_form_body_handler_preserves_headers_items_and_route_parameters_alongside_parsed_body()
    {
        HttpContextPhaseFlags extractorPhase = 0;
        bool handlerSawHeader = false;
        std::string handlerItemValue;
        std::vector<RouteParameters> capturedParams;
        std::vector<std::string> capturedBodyValues;

        auto handler = std::make_unique<FormBodyHandler>(
            [&handlerSawHeader, &handlerItemValue, &capturedParams, &capturedBodyValues](HttpRequestContext &context, RouteParameters &&params, WebUtility::QueryParameters &&body) -> IHttpHandler::HandlerResult
            {
                handlerSawHeader = context.headers().exists("X-Test", "present");
                handlerItemValue = std::any_cast<std::string>(context.items().at("route-key"));
                capturedParams.push_back(std::move(params));
                const auto value = body.get("name");
                capturedBodyValues.push_back(value.has_value() ? *value : std::string());
                return nullptr;
            },
            [&extractorPhase](HttpRequestContext &context) -> RouteParameters
            {
                extractorPhase = context.completedPhases();
                context.items()["route-key"] = std::string("route-value");
                return {{"resource", "route"}, {"id", "123"}};
            });

        RequestHarness harness(std::move(handler));

        harness.beginRequest();
        harness.addHeader("X-Test", "present");
        harness.completeHeaders();
        harness.addBody("name=Jane");
        harness.completeMessage();

        TEST_ASSERT_TRUE((extractorPhase & HttpContextPhase::CompletedReadingMessage) != 0);
        TEST_ASSERT_TRUE(handlerSawHeader);
        TEST_ASSERT_EQUAL_STRING("route-value", handlerItemValue.c_str());
        TEST_ASSERT_EQUAL_UINT64(2, capturedParams[0].size());
        TEST_ASSERT_EQUAL_STRING("route", capturedParams[0].at("resource").c_str());
        TEST_ASSERT_EQUAL_STRING("123", capturedParams[0].at("id").c_str());
        TEST_ASSERT_EQUAL_STRING("Jane", capturedBodyValues[0].c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_try_parse_header_length_accepts_complete_numeric_values);
        RUN_TEST(test_try_parse_header_length_rejects_invalid_or_partial_values);
        RUN_TEST(test_buffering_handler_does_not_buffer_when_disabled);
        RUN_TEST(test_buffering_handler_reinvokes_handle_body_on_completion_after_content_length_is_reached);
        RUN_TEST(test_buffering_handler_invokes_handle_body_from_handle_step_without_content_length);
        RUN_TEST(test_buffering_handler_truncates_to_configured_max_buffer_and_replays_on_completion);
        RUN_TEST(test_buffering_handler_ignores_empty_body_and_zero_content_length);
        RUN_TEST(test_buffered_string_body_handler_preserves_exact_bytes_in_std_string);
        RUN_TEST(test_buffered_string_body_handler_ignores_empty_payloads);
        RUN_TEST(test_raw_body_handler_preserves_chunks_and_completion_metadata);
        RUN_TEST(test_raw_body_handler_passes_large_chunks_without_internal_truncation);
        RUN_TEST(test_form_body_handler_parses_repeated_empty_and_malformed_values);
        RUN_TEST(test_form_body_handler_ignores_empty_payloads);
        RUN_TEST(test_multipart_handler_requires_boundary_header);
        RUN_TEST(test_multipart_handler_parses_named_part_and_emits_completed_event);
        RUN_TEST(test_multipart_handler_skips_empty_parts_and_only_emits_completed_event);
        RUN_TEST(test_multipart_handler_drops_small_truncated_part_payload_before_completed_event);
    #if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
        RUN_TEST(test_json_body_handler_parses_valid_json_and_preserves_route_parameters);
        RUN_TEST(test_json_body_handler_passes_empty_documents_for_malformed_payloads);
        RUN_TEST(test_json_body_handler_ignores_empty_payloads);
    #else
        RUN_TEST(test_json_body_handler_is_unavailable_in_json_disabled_build);
    #endif
        RUN_TEST(test_raw_body_handler_extractor_runs_on_first_body_chunk_phase);
        RUN_TEST(test_form_body_handler_preserves_headers_items_and_route_parameters_alongside_parsed_body);
        return UNITY_END();
    }
}

int run_test_body_handlers()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "body handlers",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
