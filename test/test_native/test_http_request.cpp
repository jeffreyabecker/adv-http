#include "../support/include/ConsolidatedNativeSuite.h"
#include "../support/include/HttpTestFixtures.h"

#include <unity.h>

#include "../../src/core/HttpRequest.h"
#include "../../src/core/HttpRequestPhase.h"
#include "../../src/server/HttpServerBase.h"

#include <memory>
#include <string>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class CapturingHandler : public IHttpHandler
    {
    public:
        HandlerResult handleStep(HttpRequest &context) override
        {
            seenMethods_.push_back(std::string(context.methodView()));
            seenPhases_.push_back(context.completedPhases());
            return nullptr;
        }

        void handleBodyChunk(HttpRequest &context, const uint8_t *, std::size_t) override
        {
            bodyChunkMethods_.push_back(std::string(context.methodView()));
            bodyChunkPhases_.push_back(context.completedPhases());
        }

        const std::vector<std::string> &seenMethods() const
        {
            return seenMethods_;
        }

        const std::vector<HttpRequestPhaseFlags> &seenPhases() const
        {
            return seenPhases_;
        }

        const std::vector<std::string> &bodyChunkMethods() const
        {
            return bodyChunkMethods_;
        }

        const std::vector<HttpRequestPhaseFlags> &bodyChunkPhases() const
        {
            return bodyChunkPhases_;
        }

    private:
        std::vector<std::string> seenMethods_;
        std::vector<HttpRequestPhaseFlags> seenPhases_;
        std::vector<std::string> bodyChunkMethods_;
        std::vector<HttpRequestPhaseFlags> bodyChunkPhases_;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_http_request_preserves_custom_method_through_factory_and_handler_steps()
    {
        HttpServerBase server(std::make_unique<TestSupport::FakeServer>());
        std::unique_ptr<CapturingHandler> capturingHandler = std::make_unique<CapturingHandler>();
        CapturingHandler *capturingHandlerPtr = capturingHandler.get();
        std::vector<std::string> factoryMethods;
        std::vector<HttpRequestPhaseFlags> factoryPhases;

        TestSupport::RecordingRequestHandlerFactory factory(
            [&factoryMethods, &factoryPhases, &capturingHandler](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                factoryMethods.push_back(std::string(context.methodView()));
                factoryPhases.push_back(context.completedPhases());
                return std::move(capturingHandler);
            });

        auto pipelineHandler = HttpRequest::createPipelineHandler(server, factory);

        const std::uint8_t body[] = {'o', 'k'};

        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onMessageBegin("MKCOL", 1, 1, "/dav/collection"));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onHeader("Host", "example.test"));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onHeadersComplete());
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onBody(body, sizeof(body)));
        TEST_ASSERT_EQUAL_INT(0, pipelineHandler->onMessageComplete());

        TEST_ASSERT_EQUAL_UINT64(1, factory.createCount());
        TEST_ASSERT_EQUAL_UINT64(1, factoryMethods.size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", factoryMethods[0].c_str());
        TEST_ASSERT_TRUE((factoryPhases[0] & HttpRequestPhase::CompletedStartingLine) != 0);
        TEST_ASSERT_NOT_NULL(factory.lastCreateContext());
        TEST_ASSERT_EQUAL_STRING("MKCOL", factory.lastCreateContext()->method());
        TEST_ASSERT_EQUAL_STRING("/dav/collection", std::string(factory.lastCreateContext()->url()).c_str());

        TEST_ASSERT_NOT_NULL(capturingHandlerPtr);
        TEST_ASSERT_EQUAL_UINT64(5, capturingHandlerPtr->seenMethods().size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[0].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[1].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[2].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[3].c_str());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->seenMethods()[4].c_str());
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[0] & HttpRequestPhase::CompletedStartingLine) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[1] & HttpRequestPhase::CompletedReadingHeaders) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[2] & HttpRequestPhase::BeginReadingBody) != 0);
        TEST_ASSERT_EQUAL_UINT64(1, capturingHandlerPtr->bodyChunkMethods().size());
        TEST_ASSERT_EQUAL_STRING("MKCOL", capturingHandlerPtr->bodyChunkMethods()[0].c_str());
        TEST_ASSERT_TRUE((capturingHandlerPtr->bodyChunkPhases()[0] & HttpRequestPhase::BeginReadingBody) != 0);
        TEST_ASSERT_TRUE((capturingHandlerPtr->seenPhases()[4] & HttpRequestPhase::CompletedReadingMessage) != 0);
        TEST_ASSERT_TRUE((factory.lastCreateContext()->completedPhases() & HttpRequestPhase::CompletedReadingMessage) != 0);
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_http_request_preserves_custom_method_through_factory_and_handler_steps);
        return UNITY_END();
    }
}

int run_test_http_request()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "http request",
        runUnitySuite,
        localSetUp,
        localTearDown);
}