#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include\HttpTestFixtures.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include "../../src/lumalink/http/pipeline/PipelineError.h"

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
    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_recording_request_handler_factory_records_response_status_and_body()
    {
        lumalink::http::TestSupport::RecordingRequestHandlerFactory factory;

        auto response = factory.createResponse(HttpStatus::Created(), std::string_view("created"));

        TEST_ASSERT_NOT_NULL(response.get());
        TEST_ASSERT_EQUAL_UINT64(1, factory.responseCreateCount());
        TEST_ASSERT_EQUAL_UINT16(201, factory.responseStatuses()[0].code());
        TEST_ASSERT_EQUAL_STRING("created", factory.lastResponseBody().c_str());

        auto body = response->getBody();
        TEST_ASSERT_EQUAL_STRING("created", lumalink::http::TestSupport::ReadByteSourceAsStdString(*body).c_str());
    }

    void test_recording_request_handler_factory_supports_local_response_overloads()
    {
        lumalink::http::TestSupport::RecordingRequestHandlerFactory factory;

        factory.createResponse(HttpStatus::Ok(), "first");
        factory.createResponse(HttpStatus::Accepted(), std::string_view("second"));

        TEST_ASSERT_EQUAL_UINT64(2, factory.responseCreateCount());
        TEST_ASSERT_EQUAL_UINT16(200, factory.responseStatuses()[0].code());
        TEST_ASSERT_EQUAL_UINT16(202, factory.responseStatuses()[1].code());
        TEST_ASSERT_EQUAL_STRING("first", factory.responseBodies()[0].c_str());
        TEST_ASSERT_EQUAL_STRING("second", factory.responseBodies()[1].c_str());
    }

    void test_capture_response_collects_status_headers_and_body()
    {
        auto response = StringResponse::create(
            HttpStatus::Ok(),
            std::string("payload"),
            {HttpHeader::ContentType("text/plain"), HttpHeader::Connection("close")});

        const lumalink::http::TestSupport::CapturedResponse captured = lumalink::http::TestSupport::CaptureResponse(std::move(response));

        TEST_ASSERT_EQUAL_UINT16(200, captured.status.code());
        TEST_ASSERT_GREATER_OR_EQUAL_UINT64(2, captured.headers.size());
        TEST_ASSERT_EQUAL_STRING("payload", captured.body.c_str());

        const auto contentType = lumalink::http::TestSupport::FindCapturedHeader(captured, "content-type");
        TEST_ASSERT_TRUE(contentType.has_value());
        TEST_ASSERT_EQUAL_STRING("text/plain", contentType->c_str());

        const auto connection = lumalink::http::TestSupport::FindCapturedHeader(captured, "Connection");
        TEST_ASSERT_TRUE(connection.has_value());
        TEST_ASSERT_EQUAL_STRING("close", connection->c_str());
    }

    void test_pipeline_event_recorder_tracks_events_addresses_and_response_callback()
    {
        lumalink::http::TestSupport::PipelineEventRecorder recorder;
        std::string deliveredBody;

        const std::uint8_t bodyBytes[] = {'o', 'k'};
        TEST_ASSERT_EQUAL_INT(0, recorder.onMessageBegin("POST", 1, 1, "/submit"));
        recorder.setAddresses("10.0.0.2", 2345, "10.0.0.1", 8080);
        TEST_ASSERT_EQUAL_INT(0, recorder.onHeader("Content-Type", "text/plain"));
        TEST_ASSERT_EQUAL_INT(0, recorder.onHeadersComplete());
        TEST_ASSERT_EQUAL_INT(0, recorder.onBody(bodyBytes, sizeof(bodyBytes)));
        TEST_ASSERT_EQUAL_INT(0, recorder.onMessageComplete());
        recorder.onResponseStarted();
        recorder.onResponseCompleted();
        recorder.onClientDisconnected();
        recorder.onError(PipelineError(PipelineErrorCode::BadRequest));
        recorder.emitResponseResult(std::make_unique<lumalink::http::TestSupport::ScriptedByteSource>(lumalink::http::TestSupport::ScriptedByteSource::FromText("response")));

        TEST_ASSERT_TRUE(recorder.hasPendingResult());
        RequestHandlingResult result = recorder.takeResult();
        TEST_ASSERT_TRUE(result.kind == RequestHandlingResult::Kind::Response);
        TEST_ASSERT_NOT_NULL(result.responseStream.get());
        deliveredBody = lumalink::http::TestSupport::ReadByteSourceAsStdString(*result.responseStream);

        TEST_ASSERT_EQUAL_STRING("POST", recorder.method().c_str());
        TEST_ASSERT_EQUAL_STRING("/submit", recorder.url().c_str());
        TEST_ASSERT_EQUAL_UINT16(1, recorder.versionMajor());
        TEST_ASSERT_EQUAL_UINT16(1, recorder.versionMinor());
        TEST_ASSERT_EQUAL_STRING("10.0.0.2", recorder.remoteAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(2345, recorder.remotePort());
        TEST_ASSERT_EQUAL_STRING("10.0.0.1", recorder.localAddress().c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, recorder.localPort());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.headers().size());
        TEST_ASSERT_EQUAL_STRING("Content-Type", recorder.headers()[0].first.c_str());
        TEST_ASSERT_EQUAL_STRING("text/plain", recorder.headers()[0].second.c_str());
        TEST_ASSERT_EQUAL_STRING("ok", recorder.bodyText().c_str());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.headersCompleteCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.messageCompleteCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.responseStartedCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.responseCompletedCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.clientDisconnectedCount());
        TEST_ASSERT_EQUAL_UINT64(1, recorder.errors().size());
        TEST_ASSERT_TRUE(recorder.errors()[0] == PipelineErrorCode::BadRequest);
        TEST_ASSERT_EQUAL_STRING("response", deliveredBody.c_str());
        TEST_ASSERT_EQUAL_UINT64(11, recorder.events().size());
        TEST_ASSERT_TRUE(recorder.events()[0].kind == lumalink::http::TestSupport::PipelineEventKind::MessageBegin);
        TEST_ASSERT_TRUE(recorder.events()[1].kind == lumalink::http::TestSupport::PipelineEventKind::AddressesSet);
        TEST_ASSERT_TRUE(recorder.events()[2].kind == lumalink::http::TestSupport::PipelineEventKind::Header);
        TEST_ASSERT_TRUE(recorder.events()[10].kind == lumalink::http::TestSupport::PipelineEventKind::RequestResultDelivered);
    }

    void test_fake_client_supports_scripted_reads_partial_writes_and_disconnect_tracking()
    {
        lumalink::http::TestSupport::FakeClient client({{"ab", false}, {"", true}, {"cd", false}}, "192.168.1.20", 4000, "192.168.1.10", 8080);
        client.setWriteScript({2, 3});
        client.setTimeout(250);

        const std::string drained = lumalink::http::TestSupport::DrainByteSourceWithAvailability(client);
        const std::size_t firstWrite = client.write(std::string_view("hello"));
        const std::size_t secondWrite = client.write(std::string_view("abc"));
        client.flush();
        client.disconnect();

        TEST_ASSERT_EQUAL_STRING("abcd", drained.c_str());
        TEST_ASSERT_EQUAL_UINT64(2, firstWrite);
        TEST_ASSERT_EQUAL_UINT64(3, secondWrite);
        TEST_ASSERT_EQUAL_STRING("heabc", client.writtenText().c_str());
        TEST_ASSERT_EQUAL_UINT64(2, client.writeSizes().size());
        TEST_ASSERT_EQUAL_UINT64(2, client.writeSizes()[0]);
        TEST_ASSERT_EQUAL_UINT64(3, client.writeSizes()[1]);
        TEST_ASSERT_EQUAL_UINT64(1, client.flushCount());
        TEST_ASSERT_EQUAL_UINT32(250, client.getTimeout());
        TEST_ASSERT_EQUAL_STRING("192.168.1.20", std::string(client.remoteAddress()).c_str());
        TEST_ASSERT_EQUAL_UINT16(4000, client.remotePort());
        TEST_ASSERT_EQUAL_STRING("192.168.1.10", std::string(client.localAddress()).c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, client.localPort());
        TEST_ASSERT_FALSE(client.stopped());
        TEST_ASSERT_FALSE(client.connected());

        client.stop();
        TEST_ASSERT_TRUE(client.stopped());
        TEST_ASSERT_EQUAL_UINT64(1, client.stopCount());
    }

    void test_fake_client_remains_connected_while_unread_buffered_input_exists()
    {
        lumalink::http::TestSupport::FakeClient client({{"xyz", false}});
        std::uint8_t oneByte[1] = {};

        client.disconnect();
        TEST_ASSERT_TRUE(client.connected());

        TEST_ASSERT_EQUAL_UINT64(1, client.read(std::span<std::uint8_t>(oneByte, 1)));
        TEST_ASSERT_EQUAL_UINT8('x', oneByte[0]);
        TEST_ASSERT_TRUE(client.connected());

        TEST_ASSERT_EQUAL_UINT64(1, client.read(std::span<std::uint8_t>(oneByte, 1)));
        TEST_ASSERT_EQUAL_UINT64(1, client.read(std::span<std::uint8_t>(oneByte, 1)));
        TEST_ASSERT_FALSE(client.connected());
    }

    void test_fake_server_tracks_begin_accept_end_and_preserves_client_order()
    {
        lumalink::http::TestSupport::FakeServer server("10.0.0.1", 8080);
        server.enqueue(std::make_unique<lumalink::http::TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{"first", false}}, "10.0.0.2", 1001, "10.0.0.1", 8080));
        server.enqueue(std::make_unique<lumalink::http::TestSupport::FakeClient>(std::initializer_list<std::pair<const char *, bool>>{{"second", false}}, "10.0.0.3", 1002, "10.0.0.1", 8080));

        server.begin();

        std::unique_ptr<IClient> first = server.accept();
        std::unique_ptr<IClient> second = server.accept();
        std::unique_ptr<IClient> empty = server.accept();
        server.end();

        TEST_ASSERT_TRUE(server.began());
        TEST_ASSERT_TRUE(server.ended());
        TEST_ASSERT_EQUAL_UINT64(1, server.beginCount());
        TEST_ASSERT_EQUAL_UINT64(1, server.endCount());
        TEST_ASSERT_EQUAL_UINT64(3, server.acceptCount());
        TEST_ASSERT_EQUAL_UINT64(0, server.pendingCount());
        TEST_ASSERT_EQUAL_STRING("10.0.0.1", std::string(server.localAddress()).c_str());
        TEST_ASSERT_EQUAL_UINT16(8080, server.port());

        TEST_ASSERT_NOT_NULL(first.get());
        TEST_ASSERT_NOT_NULL(second.get());
        TEST_ASSERT_NULL(empty.get());
        TEST_ASSERT_EQUAL_STRING("10.0.0.2", std::string(first->remoteAddress()).c_str());
        TEST_ASSERT_EQUAL_STRING("10.0.0.3", std::string(second->remoteAddress()).c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_recording_request_handler_factory_records_response_status_and_body);
        RUN_TEST(test_recording_request_handler_factory_supports_local_response_overloads);
        RUN_TEST(test_capture_response_collects_status_headers_and_body);
        RUN_TEST(test_pipeline_event_recorder_tracks_events_addresses_and_response_callback);
        RUN_TEST(test_fake_client_supports_scripted_reads_partial_writes_and_disconnect_tracking);
        RUN_TEST(test_fake_client_remains_connected_while_unread_buffered_input_exists);
        RUN_TEST(test_fake_server_tracks_begin_accept_end_and_preserves_client_order);
        return UNITY_END();
    }
}

int run_test_fixture_support()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "fixture support",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
