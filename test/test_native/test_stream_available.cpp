#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/Availability.h"
#include "../../src/streams/ByteStream.h"

using namespace HttpServerAdvanced;

namespace
{
    class FixedAvailableByteSource : public IByteSource
    {
    public:
        explicit FixedAvailableByteSource(AvailableResult result)
            : result_(result)
        {
        }

        AvailableResult available() override
        {
            return result_;
        }

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            (void)buffer;
            return 0;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            (void)buffer;
            return 0;
        }

    private:
        AvailableResult result_;
    };

    static AvailableResult mapLegacyAvailable(int availableValue, bool connected)
    {
        AvailableResult result;
        result.count = 0;
        result.errorCode = 0;
        if (availableValue > 0)
        {
            result.state = AvailabilityState::HasBytes;
            result.count = static_cast<std::size_t>(availableValue);
        }
        else if (availableValue == 0)
        {
            result.state = connected ? AvailabilityState::TemporarilyUnavailable : AvailabilityState::Exhausted;
        }
        else
        {
            result.state = AvailabilityState::Error;
            result.errorCode = availableValue;
        }

        return result;
    }

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_mapping_hasbytes()
    {
        auto result = mapLegacyAvailable(5, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::HasBytes), static_cast<int>(result.state));
        TEST_ASSERT_EQUAL_UINT64(5, result.count);
    }

    void test_mapping_exhausted()
    {
        auto result = mapLegacyAvailable(0, false);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Exhausted), static_cast<int>(result.state));
    }

    void test_mapping_temporarily_unavailable()
    {
        auto result = mapLegacyAvailable(0, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::TemporarilyUnavailable), static_cast<int>(result.state));
    }

    void test_mapping_error()
    {
        auto result = mapLegacyAvailable(-2, true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Error), static_cast<int>(result.state));
        TEST_ASSERT_EQUAL_INT(-2, result.errorCode);
    }

    void test_legacy_available_from_result_maps_has_bytes()
    {
        TEST_ASSERT_EQUAL_INT(7, LegacyAvailableFromResult(AvailableBytes(7)));
    }

    void test_legacy_available_from_result_maps_exhausted()
    {
        TEST_ASSERT_EQUAL_INT(0, LegacyAvailableFromResult(ExhaustedResult()));
    }

    void test_legacy_available_from_result_maps_temporarily_unavailable()
    {
        TEST_ASSERT_EQUAL_INT(-1, LegacyAvailableFromResult(TemporarilyUnavailableResult()));
    }

    void test_span_byte_source_read_and_peek()
    {
        const std::string text = "hello";
        SpanByteSource source{std::string_view(text)};
        uint8_t peekBuffer[2] = {};
        uint8_t readBuffer[3] = {};

        TEST_ASSERT_EQUAL_UINT64(5, source.available().count);
        TEST_ASSERT_EQUAL_UINT64(2, source.peek(HttpServerAdvanced::span<uint8_t>(peekBuffer, 2)));
        TEST_ASSERT_EQUAL_UINT8('h', peekBuffer[0]);
        TEST_ASSERT_EQUAL_UINT8('e', peekBuffer[1]);
        TEST_ASSERT_EQUAL_UINT64(3, source.read(HttpServerAdvanced::span<uint8_t>(readBuffer, 3)));
        TEST_ASSERT_EQUAL_UINT8('h', readBuffer[0]);
        TEST_ASSERT_EQUAL_UINT8('e', readBuffer[1]);
        TEST_ASSERT_EQUAL_UINT8('l', readBuffer[2]);
        TEST_ASSERT_EQUAL_UINT64(2, source.available().count);
    }

    void test_concat_byte_source_advances_across_exhausted_sources()
    {
        std::vector<std::unique_ptr<IByteSource>> sources;
        sources.emplace_back(std::make_unique<FixedAvailableByteSource>(ExhaustedResult()));
        sources.emplace_back(std::make_unique<SpanByteSource>(std::string_view("ok")));

        ConcatByteSource source(std::move(sources));
        uint8_t buffer[2] = {};

        TEST_ASSERT_TRUE(source.available().hasBytes());
        TEST_ASSERT_EQUAL_UINT64(2, source.read(HttpServerAdvanced::span<uint8_t>(buffer, 2)));
        TEST_ASSERT_EQUAL_UINT8('o', buffer[0]);
        TEST_ASSERT_EQUAL_UINT8('k', buffer[1]);
        TEST_ASSERT_TRUE(source.available().isExhausted());
    }

    void test_concat_byte_source_propagates_temporarily_unavailable()
    {
        std::vector<std::unique_ptr<IByteSource>> sources;
        sources.emplace_back(std::make_unique<FixedAvailableByteSource>(TemporarilyUnavailableResult()));

        ConcatByteSource source(std::move(sources));

        TEST_ASSERT_TRUE(source.available().isTemporarilyUnavailable());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_mapping_hasbytes);
        RUN_TEST(test_mapping_exhausted);
        RUN_TEST(test_mapping_temporarily_unavailable);
        RUN_TEST(test_mapping_error);
        RUN_TEST(test_legacy_available_from_result_maps_has_bytes);
        RUN_TEST(test_legacy_available_from_result_maps_exhausted);
        RUN_TEST(test_legacy_available_from_result_maps_temporarily_unavailable);
        RUN_TEST(test_span_byte_source_read_and_peek);
        RUN_TEST(test_concat_byte_source_advances_across_exhausted_sources);
        RUN_TEST(test_concat_byte_source_propagates_temporarily_unavailable);
        return UNITY_END();
    }
}

int run_test_stream_available()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "stream available",
        runUnitySuite,
        localSetUp,
        localTearDown);
}