#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/compat/Availability.h"
#include "../../src/streams/ByteStream.h"

using namespace HttpServerAdvanced;

namespace
{
    class LegacyAvailableStream : public Stream
    {
    public:
        using Stream::write;

        explicit LegacyAvailableStream(int availableValue)
            : availableValue_(availableValue)
        {
        }

        int available() override
        {
            return availableValue_;
        }

        int read() override
        {
            return -1;
        }

        int peek() override
        {
            return -1;
        }

        std::size_t write(uint8_t) override
        {
            return 0;
        }

    private:
        int availableValue_;
    };

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

        int read() override
        {
            return -1;
        }

        int peek() override
        {
            return -1;
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

    void test_stream_byte_source_adapter_maps_positive_available_to_has_bytes()
    {
        LegacyAvailableStream stream(7);
        StreamByteSourceAdapter adapter(stream);

        const AvailableResult result = adapter.available();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::HasBytes), static_cast<int>(result.state));
        TEST_ASSERT_EQUAL_UINT64(7, result.count);
    }

    void test_stream_byte_source_adapter_maps_zero_available_to_exhausted()
    {
        LegacyAvailableStream stream(0);
        StreamByteSourceAdapter adapter(stream);

        const AvailableResult result = adapter.available();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::Exhausted), static_cast<int>(result.state));
    }

    void test_stream_byte_source_adapter_maps_negative_available_to_temporarily_unavailable()
    {
        LegacyAvailableStream stream(-1);
        StreamByteSourceAdapter adapter(stream);

        const AvailableResult result = adapter.available();
        TEST_ASSERT_EQUAL_INT(static_cast<int>(AvailabilityState::TemporarilyUnavailable), static_cast<int>(result.state));
    }

    void test_byte_source_stream_adapter_maps_exhausted_to_zero_available()
    {
        ByteSourceStreamAdapter adapter(std::make_unique<FixedAvailableByteSource>(ExhaustedResult()));

        TEST_ASSERT_EQUAL_INT(0, adapter.available());
    }

    void test_byte_source_stream_adapter_maps_temporarily_unavailable_to_negative_available()
    {
        ByteSourceStreamAdapter adapter(std::make_unique<FixedAvailableByteSource>(TemporarilyUnavailableResult()));

        TEST_ASSERT_EQUAL_INT(-1, adapter.available());
    }

    void test_byte_source_stream_adapter_maps_error_to_negative_available()
    {
        ByteSourceStreamAdapter adapter(std::make_unique<FixedAvailableByteSource>(ErrorResult(-9)));

        TEST_ASSERT_EQUAL_INT(-1, adapter.available());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_mapping_hasbytes);
        RUN_TEST(test_mapping_exhausted);
        RUN_TEST(test_mapping_temporarily_unavailable);
        RUN_TEST(test_mapping_error);
        RUN_TEST(test_stream_byte_source_adapter_maps_positive_available_to_has_bytes);
        RUN_TEST(test_stream_byte_source_adapter_maps_zero_available_to_exhausted);
        RUN_TEST(test_stream_byte_source_adapter_maps_negative_available_to_temporarily_unavailable);
        RUN_TEST(test_byte_source_stream_adapter_maps_exhausted_to_zero_available);
        RUN_TEST(test_byte_source_stream_adapter_maps_temporarily_unavailable_to_negative_available);
        RUN_TEST(test_byte_source_stream_adapter_maps_error_to_negative_available);
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