#include "../support/include/ConsolidatedNativeSuite.h"

#include "../../src/lumalink/http/LumaLinkHttp.h"

#include <unity.h>

#include "../../src/lumalink/http/websocket/WebSocketFrameCodec.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

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

    std::vector<std::uint8_t> BuildMaskedClientFrame(WebSocketOpcode opcode, bool fin, std::string_view payload)
    {
        std::vector<std::uint8_t> frame;
        const std::array<std::uint8_t, 4> mask = {0x37, 0xFA, 0x21, 0x3D};

        frame.push_back(static_cast<std::uint8_t>((fin ? 0x80U : 0x00U) | static_cast<std::uint8_t>(opcode)));
        frame.push_back(static_cast<std::uint8_t>(0x80U | payload.size()));
        frame.insert(frame.end(), mask.begin(), mask.end());

        for (std::size_t i = 0; i < payload.size(); ++i)
        {
            frame.push_back(static_cast<std::uint8_t>(payload[i]) ^ mask[i % 4]);
        }

        return frame;
    }

    void test_websocket_frame_parser_parses_masked_text_frame_and_unmasks_payload()
    {
        WebSocketFrameParser parser(true);
        const std::vector<std::uint8_t> frame = BuildMaskedClientFrame(WebSocketOpcode::Text, true, "Hello");

        const WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(frame.data(), frame.size()));

        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_TRUE(result.frameReady);
        TEST_ASSERT_EQUAL_UINT64(frame.size(), result.bytesConsumed);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketOpcode::Text), static_cast<int>(result.frame.header.opcode));
        TEST_ASSERT_TRUE(result.frame.header.fin);
        TEST_ASSERT_TRUE(result.frame.header.masked);
        TEST_ASSERT_EQUAL_UINT64(5, result.frame.payload.size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const std::uint8_t *>("Hello"), result.frame.payload.data(), 5);
    }

    void test_websocket_frame_parser_supports_incremental_header_mask_and_payload_reads()
    {
        WebSocketFrameParser parser(true);
        const std::vector<std::uint8_t> frame = BuildMaskedClientFrame(WebSocketOpcode::Binary, true, "abc");

        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(frame.data(), 1));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::NeedMoreData), static_cast<int>(result.status));

        result = parser.parse(std::span<const std::uint8_t>(frame.data() + 1, 3));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::NeedMoreData), static_cast<int>(result.status));

        result = parser.parse(std::span<const std::uint8_t>(frame.data() + 4, frame.size() - 4));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_TRUE(result.frameReady);
        TEST_ASSERT_EQUAL_UINT64(3, result.frame.payload.size());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const std::uint8_t *>("abc"), result.frame.payload.data(), 3);
    }

    void test_websocket_frame_parser_rejects_unmasked_client_frames_and_reserved_bits()
    {
        WebSocketFrameParser parser(true);

        const std::uint8_t unmasked[] = {0x81, 0x02, 'o', 'k'};
        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(unmasked, sizeof(unmasked)));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::UnmaskedClientFrame), static_cast<int>(result.error));

        parser.reset();
        const std::uint8_t reservedBitSet[] = {0xC1, 0x80, 0x00, 0x00, 0x00, 0x00};
        result = parser.parse(std::span<const std::uint8_t>(reservedBitSet, sizeof(reservedBitSet)));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::InvalidReservedBits), static_cast<int>(result.error));
    }

    void test_websocket_frame_parser_rejects_fragmented_control_frames_and_orphan_continuation()
    {
        WebSocketFrameParser parser(true);

        const std::vector<std::uint8_t> fragmentedPing = BuildMaskedClientFrame(WebSocketOpcode::Ping, false, "hi");
        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(fragmentedPing.data(), fragmentedPing.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::ControlFrameFragmented), static_cast<int>(result.error));

        parser.reset();
        const std::vector<std::uint8_t> orphanContinuation = BuildMaskedClientFrame(WebSocketOpcode::Continuation, true, "x");
        result = parser.parse(std::span<const std::uint8_t>(orphanContinuation.data(), orphanContinuation.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::UnexpectedContinuationFrame), static_cast<int>(result.error));
    }

    void test_websocket_frame_parser_rejects_interrupted_continuation_sequences()
    {
        WebSocketFrameParser parser(true);

        const std::vector<std::uint8_t> firstFragment = BuildMaskedClientFrame(WebSocketOpcode::Text, false, "hel");
        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(firstFragment.data(), firstFragment.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));

        const std::vector<std::uint8_t> interruptedByData = BuildMaskedClientFrame(WebSocketOpcode::Binary, true, "!");
        result = parser.parse(std::span<const std::uint8_t>(interruptedByData.data(), interruptedByData.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::InterruptedContinuationSequence), static_cast<int>(result.error));
    }

    void test_websocket_frame_parser_parses_extended_payload_lengths_and_rejects_malformed_variants()
    {
        WebSocketFrameParser parser(false);

        std::vector<std::uint8_t> payload126(126, 0x41);
        std::vector<std::uint8_t> frame126;
        frame126.push_back(0x82);
        frame126.push_back(126);
        frame126.push_back(0x00);
        frame126.push_back(126);
        frame126.insert(frame126.end(), payload126.begin(), payload126.end());

        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(frame126.data(), frame126.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT64(126, result.frame.payload.size());

        parser.reset();
        std::vector<std::uint8_t> malformed126 = {0x82, 126, 0x00, 0x7D};
        malformed126.push_back('x');
        result = parser.parse(std::span<const std::uint8_t>(malformed126.data(), malformed126.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::MalformedExtendedPayloadLength), static_cast<int>(result.error));

        parser.reset();
        std::vector<std::uint8_t> payload65536(65536, 0x42);
        std::vector<std::uint8_t> frame65536;
        frame65536.push_back(0x82);
        frame65536.push_back(127);
        frame65536.push_back(0x00);
        frame65536.push_back(0x00);
        frame65536.push_back(0x00);
        frame65536.push_back(0x00);
        frame65536.push_back(0x00);
        frame65536.push_back(0x01);
        frame65536.push_back(0x00);
        frame65536.push_back(0x00);
        frame65536.insert(frame65536.end(), payload65536.begin(), payload65536.end());

        result = parser.parse(std::span<const std::uint8_t>(frame65536.data(), frame65536.size()));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT64(65536, result.frame.payload.size());
    }

    void test_websocket_frame_parser_validates_close_payload_lengths()
    {
        WebSocketFrameParser parser(false);

        const std::uint8_t emptyClose[] = {0x88, 0x00};
        WebSocketParseResult result = parser.parse(std::span<const std::uint8_t>(emptyClose, sizeof(emptyClose)));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT64(0, result.frame.payload.size());

        parser.reset();
        const std::uint8_t malformedClose[] = {0x88, 0x01, 0x03};
        result = parser.parse(std::span<const std::uint8_t>(malformedClose, sizeof(malformedClose)));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::MalformedClosePayload), static_cast<int>(result.error));

        parser.reset();
        const std::uint8_t validClose[] = {0x88, 0x02, 0x03, 0xE8};
        result = parser.parse(std::span<const std::uint8_t>(validClose, sizeof(validClose)));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT64(2, result.frame.payload.size());
    }

    void test_websocket_frame_serializer_encodes_payload_lengths_at_boundaries()
    {
        std::vector<std::uint8_t> output(70000);

        const std::vector<std::uint8_t> payload125(125, 0x11);
        WebSocketSerializeResult result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(output.data(), output.size()),
            std::span<const std::uint8_t>(payload125.data(), payload125.size()),
            WebSocketOpcode::Binary,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT8(0x82, output[0]);
        TEST_ASSERT_EQUAL_UINT8(125, output[1]);

        const std::vector<std::uint8_t> payload126(126, 0x22);
        result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(output.data(), output.size()),
            std::span<const std::uint8_t>(payload126.data(), payload126.size()),
            WebSocketOpcode::Binary,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT8(126, output[1]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[2]);
        TEST_ASSERT_EQUAL_UINT8(126, output[3]);

        const std::vector<std::uint8_t> payload65535(65535, 0x33);
        result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(output.data(), output.size()),
            std::span<const std::uint8_t>(payload65535.data(), payload65535.size()),
            WebSocketOpcode::Binary,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT8(126, output[1]);
        TEST_ASSERT_EQUAL_UINT8(0xFF, output[2]);
        TEST_ASSERT_EQUAL_UINT8(0xFF, output[3]);

        const std::vector<std::uint8_t> payload65536(65536, 0x44);
        result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(output.data(), output.size()),
            std::span<const std::uint8_t>(payload65536.data(), payload65536.size()),
            WebSocketOpcode::Binary,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::Ok), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_UINT8(127, output[1]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[2]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[3]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[4]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[5]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[6]);
        TEST_ASSERT_EQUAL_UINT8(0x01, output[7]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[8]);
        TEST_ASSERT_EQUAL_UINT8(0x00, output[9]);
    }

    void test_websocket_frame_serializer_rejects_invalid_control_frames_and_small_output_buffers()
    {
        std::uint8_t tiny[3] = {};
        const std::uint8_t payload[] = {'o', 'k'};

        WebSocketSerializeResult result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(tiny, sizeof(tiny)),
            std::span<const std::uint8_t>(payload, sizeof(payload)),
            WebSocketOpcode::Ping,
            false);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::ControlFrameFragmented), static_cast<int>(result.error));

        std::vector<std::uint8_t> longPayload(130, 0x00);
        std::vector<std::uint8_t> output(256, 0x00);
        result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(output.data(), output.size()),
            std::span<const std::uint8_t>(longPayload.data(), longPayload.size()),
            WebSocketOpcode::Close,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::ProtocolError), static_cast<int>(result.status));
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecError::ControlFrameTooLarge), static_cast<int>(result.error));

        const std::uint8_t smallPayload[] = {'h', 'i'};
        result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(tiny, sizeof(tiny)),
            std::span<const std::uint8_t>(smallPayload, sizeof(smallPayload)),
            WebSocketOpcode::Binary,
            true);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(WebSocketCodecStatus::BufferTooSmall), static_cast<int>(result.status));
    }

    int runUnitySuite()
    {
        UNITY_BEGIN();
        RUN_TEST(test_websocket_frame_parser_parses_masked_text_frame_and_unmasks_payload);
        RUN_TEST(test_websocket_frame_parser_supports_incremental_header_mask_and_payload_reads);
        RUN_TEST(test_websocket_frame_parser_rejects_unmasked_client_frames_and_reserved_bits);
        RUN_TEST(test_websocket_frame_parser_rejects_fragmented_control_frames_and_orphan_continuation);
        RUN_TEST(test_websocket_frame_parser_rejects_interrupted_continuation_sequences);
        RUN_TEST(test_websocket_frame_parser_parses_extended_payload_lengths_and_rejects_malformed_variants);
        RUN_TEST(test_websocket_frame_parser_validates_close_payload_lengths);
        RUN_TEST(test_websocket_frame_serializer_encodes_payload_lengths_at_boundaries);
        RUN_TEST(test_websocket_frame_serializer_rejects_invalid_control_frames_and_small_output_buffers);
        return UNITY_END();
    }
}

int run_test_websocket_frame_codec()
{
    return lumalink::http::TestSupport::RunConsolidatedSuite(
        "websocket frame codec",
        runUnitySuite,
        localSetUp,
        localTearDown);
}
