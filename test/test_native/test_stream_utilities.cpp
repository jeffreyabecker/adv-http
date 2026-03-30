#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/HttpServerAdvanced.h"

#include <unity.h>

#include "../../src/streams/Streams.h"
#include "../../src/streams/Base64Stream.h"

#include <cstring>
#include <memory>
#include <vector>

using namespace httpadv::v1::core;
using namespace httpadv::v1::handlers;
using namespace httpadv::v1::pipeline;
using namespace httpadv::v1::response;
using namespace httpadv::v1::routing;
using namespace httpadv::v1::server;
using namespace httpadv::v1::staticfiles;
using namespace httpadv::v1::streams;
using namespace httpadv::v1::transport;
using namespace httpadv::v1::util;
using namespace httpadv::v1::websocket;

namespace
{
    class ExhaustedReadStream : public ReadStream
    {
    public:
        AvailableResult available() override
        {
            return ExhaustedResult();
        }

    protected:
        int readSingleByte() override
        {
            return -1;
        }

        int peekSingleByte() override
        {
            return -1;
        }
    };

    class NegativeAvailableReadStream : public ReadStream
    {
    public:
        explicit NegativeAvailableReadStream(const char *text)
        {
            const auto *begin = reinterpret_cast<const uint8_t *>(text);
            bytes_.assign(begin, begin + std::strlen(text));
        }

        NegativeAvailableReadStream(const uint8_t *data, std::size_t length)
            : bytes_(data, data + length)
        {
        }

        AvailableResult available() override
        {
            return position_ < bytes_.size() ? TemporarilyUnavailableResult() : ExhaustedResult();
        }

    protected:
        int readSingleByte() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return bytes_[position_++];
        }

        int peekSingleByte() override
        {
            if (position_ >= bytes_.size())
            {
                return -1;
            }

            return bytes_[position_];
        }

    private:
        std::vector<uint8_t> bytes_;
        std::size_t position_ = 0;
    };

    class TemporarilyUnavailableReadStream : public ReadStream
    {
    public:
        AvailableResult available() override
        {
            return TemporarilyUnavailableResult();
        }

    protected:
        int readSingleByte() override
        {
            return -1;
        }

        int peekSingleByte() override
        {
            return -1;
        }
    };

    void localSetUp()
    {
    }

    void localTearDown()
    {
    }

    void test_empty_read_stream_reports_end_of_stream()
    {
        ExhaustedReadStream stream;

        TEST_ASSERT_EQUAL_INT(0, LegacyAvailableFromResult(stream.available()));
        TEST_ASSERT_EQUAL_INT(-1, PeekByte(stream));
        TEST_ASSERT_EQUAL_INT(-1, ReadByte(stream));
    }

    void test_octets_stream_reads_and_peeks_without_consuming_peek()
    {
        StdStringByteSource stream(std::string("abc"));

        TEST_ASSERT_EQUAL_INT(3, LegacyAvailableFromResult(stream.available()));
        TEST_ASSERT_EQUAL_INT('a', PeekByte(stream));
        TEST_ASSERT_EQUAL_INT(3, LegacyAvailableFromResult(stream.available()));
        TEST_ASSERT_EQUAL_INT('a', ReadByte(stream));
        TEST_ASSERT_EQUAL_INT(2, LegacyAvailableFromResult(stream.available()));
        TEST_ASSERT_EQUAL_INT('b', ReadByte(stream));
        TEST_ASSERT_EQUAL_INT('c', ReadByte(stream));
        TEST_ASSERT_EQUAL_INT(0, LegacyAvailableFromResult(stream.available()));
        TEST_ASSERT_EQUAL_INT(-1, ReadByte(stream));
    }

    void test_read_helpers_convert_stream_contents()
    {
        StdStringByteSource stringStream(std::string("stream"));
        std::string result = ReadAsStdString(stringStream);
        TEST_ASSERT_EQUAL_STRING("stream", result.c_str());

        uint8_t octets[] = {1, 2, 3, 4};
        SpanByteSource vectorStream(octets, sizeof(octets));
        std::vector<uint8_t> vector = ReadAsVector(vectorStream);
        TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(vector.size()));
        TEST_ASSERT_EQUAL_UINT8(1, vector[0]);
        TEST_ASSERT_EQUAL_UINT8(4, vector[3]);
    }

    void test_read_as_string_reads_when_available_is_negative()
    {
        NegativeAvailableReadStream stream("queued");

        std::string result = ReadAsStdString(stream);

        TEST_ASSERT_EQUAL_STRING("queued", result.c_str());
    }

    void test_read_as_vector_reads_when_available_is_negative()
    {
        const uint8_t octets[] = {5, 6, 7};
        NegativeAvailableReadStream stream(octets, sizeof(octets));

        std::vector<uint8_t> result = ReadAsVector(stream);

        TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(result.size()));
        TEST_ASSERT_EQUAL_UINT8(5, result[0]);
        TEST_ASSERT_EQUAL_UINT8(7, result[2]);
    }

    void test_base64_encoder_stream_encodes_expected_output()
    {
        auto encoder = Base64EncoderStream::create("Man");
        std::string result = ReadAsStdString(encoder);

        TEST_ASSERT_EQUAL_STRING("TWFu", result.c_str());
    }

    void test_base64_decoder_stream_decodes_expected_output()
    {
        auto decoder = Base64DecoderStream::create("TWFu");
        std::string result = ReadAsStdString(decoder);

        TEST_ASSERT_EQUAL_STRING("Man", result.c_str());
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_empty_read_stream_reports_end_of_stream);
        RUN_TEST(test_octets_stream_reads_and_peeks_without_consuming_peek);
        RUN_TEST(test_read_helpers_convert_stream_contents);
        RUN_TEST(test_read_as_string_reads_when_available_is_negative);
        RUN_TEST(test_read_as_vector_reads_when_available_is_negative);
        RUN_TEST(test_base64_encoder_stream_encodes_expected_output);
        RUN_TEST(test_base64_decoder_stream_decodes_expected_output);
        return UNITY_END();
    }
}

int run_test_stream_utilities()
{
    return httpadv::v1::TestSupport::RunConsolidatedSuite(
        "stream utilities",
        runUnitySuite,
        localSetUp,
        localTearDown);
}