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
    String ReadByteSourceAsString(IByteSource &source)
    {
        String result;
        uint8_t buffer[16] = {};
        while (true)
        {
            const size_t bytesRead = source.read(HttpServerAdvanced::span<uint8_t>(buffer, sizeof(buffer)));
            if (bytesRead == 0)
            {
                break;
            }

            for (size_t index = 0; index < bytesRead; ++index)
            {
                result += static_cast<char>(buffer[index]);
            }
        }

        return result;
    }

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

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size() && position_ < bytes_.size())
            {
                buffer[totalRead++] = static_cast<uint8_t>(bytes_[position_++]);
            }

            return totalRead;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size() && (position_ + totalRead) < bytes_.size())
            {
                buffer[totalRead] = static_cast<uint8_t>(bytes_[position_ + totalRead]);
                ++totalRead;
            }

            return totalRead;
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

    void test_byte_source_reads_direct_body_content()
    {
        auto body = std::make_unique<VectorByteSource>("ok");

        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("ok"));
    }

    void test_byte_source_reports_temporarily_unavailable()
    {
        auto body = std::make_unique<VectorByteSource>("", true);

        TEST_ASSERT_TRUE(body->available().isTemporarilyUnavailable());
    }

    void test_chunked_response_body_stream_reads_from_byte_source()
    {
        auto body = ChunkedHttpResponseBodyStream::create(std::make_unique<VectorByteSource>("Hi"));

        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("2\r\nHi\r\n0\r\n\r\n"));
    }

    void test_http_response_accepts_byte_source_body()
    {
        HttpHeaderCollection headers;
        HttpResponse response(HttpStatus::Ok(), std::make_unique<VectorByteSource>("body"), std::move(headers));

        auto body = response.getBody();
        const String content = ReadByteSourceAsString(*body);
        TEST_ASSERT_TRUE(content == String("body"));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_byte_source_reads_direct_body_content);
        RUN_TEST(test_byte_source_reports_temporarily_unavailable);
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