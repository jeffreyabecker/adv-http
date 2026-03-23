// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Document/JsonDocument.hpp>
#include <ArduinoJson/Variant/VariantDataVisitor.hpp>
#include <ArduinoJson/Variant/VariantAttorney.hpp>
#include <ArduinoJson/Json/EscapeSequence.hpp>
#include <ArduinoJson/Numbers/FloatParts.hpp>
#include <ArduinoJson/Numbers/JsonInteger.hpp>
#include <ArduinoJson/Memory/ResourceManager.hpp>
#include <ArduinoJson/Polyfills/assert.hpp>
#include <ArduinoJson/Polyfills/type_traits.hpp>
#include <ArduinoJson/Polyfills/utility.hpp>

#include "streams/ByteStream.h"

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

class JsonPullSerializer {
 public:
  explicit JsonPullSerializer(JsonDocument&& doc);

  size_t read(uint8_t* output, size_t capacity);
  bool done() const;
  bool hasData() const;
  int peek();
  int available() const;

 private:
  enum class FrameType : uint8_t {
    Value,
    Array,
    Object
  };

  struct Frame {
    FrameType type;
    VariantData* data;
    SlotId slotId;
    bool started;
    bool afterChild;
    bool isKey;
  };

  enum class PendingType : uint8_t {
    None,
    Token,
    String,
    Raw
  };

  struct PendingStringState {
    const char* ptr;
    size_t size;
    size_t index;
    bool quoteOpenPending;
    bool quoteClosePending;
    char escape[6];
    uint8_t escapeLen;
    uint8_t escapePos;
  };

  static const size_t kMaxDepth = ARDUINOJSON_DEFAULT_NESTING_LIMIT;
  static const size_t kTokenCapacity = 48;

  bool pushValue(VariantData* data);
  bool beginValue(VariantData* data);
  void popFrame();

  size_t drainPending(uint8_t* output, size_t capacity);
  size_t drainPendingToken(uint8_t* output, size_t capacity);
  size_t drainPendingRaw(uint8_t* output, size_t capacity);
  size_t drainPendingString(uint8_t* output, size_t capacity);

  bool emitSingleChar(char c);
  bool prepareToken(const char* s);
  bool prepareToken(const char* s, size_t n);
  bool prepareInteger(JsonInteger value);
  bool prepareInteger(JsonUInt value);
  bool prepareFloat(JsonFloat value);
  bool prepareBool(bool value);
  bool prepareNull();
  bool prepareRaw(RawString value);
  bool prepareString(const char* s);
  bool prepareString(const char* s, size_t n);

  size_t writeUnsignedToBuffer(char* end, JsonUInt value);
  size_t writeSignedToBuffer(char* end, JsonInteger value);

  bool nextEscape(char c);

  JsonDocument doc_;
  ResourceManager* resources_;
  VariantData* root_;

  Frame stack_[kMaxDepth];
  size_t depth_;

  PendingType pendingType_;

  char token_[kTokenCapacity];
  size_t tokenBegin_;
  size_t tokenEnd_;

  PendingStringState pendingString_;

  const char* rawPtr_;
  size_t rawSize_;
  size_t rawIndex_;

  bool rootPushed_;
  bool finished_;
  int peeked_;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

class JsonSerializerStream : public HttpServerAdvanced::IByteSource {
 public:
  explicit JsonSerializerStream(JsonDocument&& doc);

  HttpServerAdvanced::AvailableResult available() override;
  size_t read(HttpServerAdvanced::span<uint8_t> buffer) override;
  size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override;



 private:
  detail::JsonPullSerializer serializer_;
};

ARDUINOJSON_END_PUBLIC_NAMESPACE