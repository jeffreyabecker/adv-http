#pragma once
#include <functional>
#include <memory>
#include <utility>


#include "../core/HttpTimeouts.h"
#include "IPipelineHandler.h"

#include "../../../llhttp/include/llhttp.h"
#include "../core/Defines.h"
#include "PipelineError.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>


namespace lumalink::http::pipeline {
enum class RequestParserEvent {
  None,
  MessageBegin,
  Url,
  HeaderField,
  HeaderValue,
  HeadersComplete,
  Body,
  MessageComplete,
  Error
};

// Wrapper class for the http_parser library
class RequestParser {
private:
  llhttp_t parser_;
  llhttp_settings_t settings_;
  RequestParserEvent currentEvent_ = RequestParserEvent::None;
  IPipelineHandler &eventHandler_;
  /**
  Recommendation
  For memory-constrained embedded devices (RP2040, ESP8266):

  Factor	Winner	Why
  Worst-case predictability	Static	No fragmentation, bounded
  Average-case efficiency	Dynamic	Typical requests use less
  Real-time/latency	Static	No malloc during parse
  Long-running stability	Static	Fragmentation accumulates over time
  Verdict: The current static buffer is the safer choice for an HTTP server that
  must run indefinitely without reboot. The ~800 B savings from dynamic
  allocation isn't worth the fragmentation risk on systems without virtual
  memory.

  If you wanted to pursue dynamic allocation, I'd suggest the hybrid approach
  with a 256-byte inline buffer—but only if profiling shows memory pressure in
  real workloads.
  */
  std::array<char, lumalink::http::core::REQUEST_PARSER_BUFFER_LENGTH> buffer_{};
  std::size_t writePos_ = 0;
  std::size_t urlPos_ = 0;
  std::size_t urlLen_ = 0;
  std::size_t methodPos_ = 0;
  std::size_t methodLen_ = 0;
  std::size_t versionPos_ = 0;
  std::size_t versionLen_ = 0;
  std::size_t headerFieldPos_ = 0;
  std::size_t headerFieldLen_ = 0;
  std::size_t headerValuePos_ = 0;
  std::size_t headerValueLen_ = 0;
  std::size_t headerCount_ = 0;
  bool finished_ = false;

  bool appendToBuffer(const char *data, std::size_t length,
                      std::size_t maxAllowed, std::size_t &startPos,
                      std::size_t &curLen);

  void resetBuffer();

  // Helper to get RequestParser* from llhttp_t
  static RequestParser *get_instance(llhttp_t *parser);

  // Static trampoline functions for llhttp_settings_t
  static int on_message_begin_cb(llhttp_t *parser);
  static int on_protocol_cb(llhttp_t *parser, const char *at,
                            std::size_t length);
  static int on_url_cb(llhttp_t *parser, const char *at, std::size_t length);
  static int on_method_cb(llhttp_t *parser, const char *at, std::size_t length);
  static int on_version_cb(llhttp_t *parser, const char *at,
                           std::size_t length);
  static int on_header_field_cb(llhttp_t *parser, const char *at,
                                std::size_t length);
  static int on_header_value_cb(llhttp_t *parser, const char *at,
                                std::size_t length);
  static int on_headers_complete_cb(llhttp_t *parser);
  static int on_body_cb(llhttp_t *parser, const char *at, std::size_t length);
  static int on_message_complete_cb(llhttp_t *parser);
  static int on_protocol_complete_cb(llhttp_t *parser);
  static int on_url_complete_cb(llhttp_t *parser);
  static int on_method_complete_cb(llhttp_t *parser);
  static int on_version_complete_cb(llhttp_t *parser);
  static int on_header_field_complete_cb(llhttp_t *parser);
  static int on_header_value_complete_cb(llhttp_t *parser);

  std::string method_;
  short versionMajor_ = 1;
  short versionMinor_ = 1;
  // Map llhttp error code to PipelineErrorCode
  static PipelineErrorCode mapLlhttpErrorToPipelineError(int llhttpError);

public:
  RequestParser(IPipelineHandler &eventHandler);

  std::size_t execute(const uint8_t *data, std::size_t length);

  void reset();

  const char *method() const { return method_.c_str(); }
  short versionMajor() const { return versionMajor_; }
  short versionMinor() const { return versionMinor_; }

  bool isFinished() const { return llhttp_message_needs_eof(&parser_) == 0; }

  bool shouldKeepAlive() const {
    return llhttp_should_keep_alive(&parser_) != 0;
  }

  RequestParserEvent currentEvent() const { return currentEvent_; }
};
} // namespace lumalink::http::pipeline
