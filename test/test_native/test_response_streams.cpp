#include "../support/include/ConsolidatedNativeSuite.h"

#include <unity.h>

#include "../../src/response/ChunkedHttpResponseBodyStream.h"
#include "../../src/response/HttpResponse.h"
#include "../../src/streams/ByteStream.h"
#include "../../src/streams/Streams.h"

#include <cstring>
#include <vector>

using namespace HttpServerAdvanced;

namespace
{
    class VectorByteSource : public IByteSource
    {
    public:
        explicit VectorByteSource(const char *text, bool temporarilyUnavailableOnce = false)
            : bytes_(text, text + std::strlen(text)), temporarilyUnavailableOnce_(temporarilyUnavailableOnce)
        {
        }

        AvailableResult available() override
        {
            if (position_ < bytes_.size())
            {
                return AvailableBytes(bytes_.size() - position_);
            }

            if (temporarilyUnavailableOnce_ && !temporarilyUnavailableReported_)
            {
                temporarilyUnavailableReported_ = true;
                return TemporarilyUnavailableResult();
            }

            return ExhaustedResult();
        }

        int read() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return static_cast<unsigned char>(bytes_[position_++]);
        }

        int peek() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return static_cast<unsigned char>(bytes_[position_]);
        }

    private:
        std::vector<char> bytes_;
        std::size_t position_ = 0;
        bool temporarilyUnavailableOnce_ = false;
        bool temporarilyUnavailableReported_ = false;
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_http_response_body_stream_reads_from_byte_source()
    {
        auto body = HttpResponseBodyStream::create(std::make_unique<VectorByteSource>("ok"));

        const String content = ReadAsString(*body);
        TEST_ASSERT_TRUE(content == String("ok"));
    }

    void test_http_response_body_stream_maps_temporarily_unavailable_to_legacy_contract()
    {
        auto body = HttpResponseBodyStream::create(std::make_unique<VectorByteSource>("", true));

        TEST_ASSERT_EQUAL_INT(-1, body->available());
    }

    void test_chunked_response_body_stream_reads_from_byte_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(std::make_unique<VectorByteSource>("Hi"));

        const String content = ReadAsString(*body);
        TEST_ASSERT_TRUE(content == String("2\r\nHi\r\n0\r\n\r\n"));
    }

    void test_http_response_accepts_byte_source_body()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<VectorByteSource>("body"), std::move(headers));

        auto body = response.getBody();
        const String content = ReadAsString(*body);
        TEST_ASSERT_TRUE(content == String("body"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_http_response_body_stream_reads_from_byte_source);
        RUN_TEST(test_http_response_body_stream_maps_temporarily_unavailable_to_legacy_contract);
        RUN_TEST(test_chunked_response_body_stream_reads_from_byte_source);
        RUN_TEST(test_http_response_accepts_byte_source_body);
        return UNITY_END();
    }
}

int run_test_response_streams()
{
    return HttpServerAdvanced::TestSupport::RunConsolidatedSuite(
        "response streams",
        runUnitySuite,
        localSetUp,
        localTearDown);
}