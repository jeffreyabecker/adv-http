// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#include "streams/JsonSerializerStream.h"

#include <ArduinoJson/Json/EscapeSequence.hpp>
#include <ArduinoJson/Namespace.hpp>
#include <ArduinoJson/Numbers/FloatParts.hpp>
#include <ArduinoJson/Variant/ConverterImpl.hpp>
#include <ArduinoJson/Variant/VariantData.hpp>
#include <ArduinoJson/Variant/JsonVariantConst.hpp>
#include <string.h>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

JsonPullSerializer::JsonPullSerializer(JsonDocument&& doc)
    : doc_(detail::move(doc)),
      resources_(VariantAttorney::getResourceManager(doc_)),
      root_(VariantAttorney::getData(doc_)),
      depth_(0),
      pendingType_(PendingType::None),
      tokenBegin_(0),
      tokenEnd_(0),
      rawPtr_(0),
      rawSize_(0),
      rawIndex_(0),
      rootPushed_(false),
      finished_(false),
      peeked_(-1) {
  pendingString_.ptr = 0;
  pendingString_.size = 0;
  pendingString_.index = 0;
  pendingString_.quoteOpenPending = false;
  pendingString_.quoteClosePending = false;
  pendingString_.escapeLen = 0;
  pendingString_.escapePos = 0;
}

bool JsonPullSerializer::done() const {
  return finished_ && pendingType_ == PendingType::None && peeked_ < 0;
}

bool JsonPullSerializer::hasData() const {
  return !done();
}

int JsonPullSerializer::available() const {
  return hasData() ? 1 : 0;
}

int JsonPullSerializer::peek() {
  if (peeked_ >= 0)
    return peeked_;

  uint8_t c;
  size_t n = const_cast<JsonPullSerializer*>(this)->read(&c, 1);
  if (n == 0)
    return -1;

  peeked_ = c;
  return peeked_;
}

bool JsonPullSerializer::pushValue(VariantData* data) {
  if (depth_ >= kMaxDepth)
    return false;

  stack_[depth_].type = FrameType::Value;
  stack_[depth_].data = data;
  stack_[depth_].slotId = NULL_SLOT;
  stack_[depth_].started = false;
  stack_[depth_].afterChild = false;
  stack_[depth_].isKey = false;
  depth_++;
  return true;
}

void JsonPullSerializer::popFrame() {
  if (depth_ > 0)
    depth_--;
}

bool JsonPullSerializer::emitSingleChar(char c) {
  token_[0] = c;
  tokenBegin_ = 0;
  tokenEnd_ = 1;
  pendingType_ = PendingType::Token;
  return true;
}

bool JsonPullSerializer::prepareToken(const char* s) {
  return prepareToken(s, strlen(s));
}

bool JsonPullSerializer::prepareToken(const char* s, size_t n) {
  ARDUINOJSON_ASSERT(n <= kTokenCapacity);
  if (n > kTokenCapacity)
    return false;
  memcpy(token_, s, n);
  tokenBegin_ = 0;
  tokenEnd_ = n;
  pendingType_ = PendingType::Token;
  return true;
}

size_t JsonPullSerializer::writeUnsignedToBuffer(char* end, JsonUInt value) {
  char* p = end;
  do {
    *--p = char(value % 10 + '0');
    value = JsonUInt(value / 10);
  } while (value);
  return size_t(end - p);
}

size_t JsonPullSerializer::writeSignedToBuffer(char* end, JsonInteger value) {
  using unsigned_type = make_unsigned_t<JsonInteger>;
  char* p = end;
  unsigned_type u;

  if (value < 0) {
    u = unsigned_type(unsigned_type(~value) + 1);
    do {
      *--p = char(u % 10 + '0');
      u = unsigned_type(u / 10);
    } while (u);
    *--p = '-';
  } else {
    u = unsigned_type(value);
    do {
      *--p = char(u % 10 + '0');
      u = unsigned_type(u / 10);
    } while (u);
  }

  return size_t(end - p);
}

bool JsonPullSerializer::prepareInteger(JsonInteger value) {
  char* end = token_ + kTokenCapacity;
  size_t n = writeSignedToBuffer(end, value);
  memmove(token_, end - n, n);
  tokenBegin_ = 0;
  tokenEnd_ = n;
  pendingType_ = PendingType::Token;
  return true;
}

bool JsonPullSerializer::prepareInteger(JsonUInt value) {
  char* end = token_ + kTokenCapacity;
  size_t n = writeUnsignedToBuffer(end, value);
  memmove(token_, end - n, n);
  tokenBegin_ = 0;
  tokenEnd_ = n;
  pendingType_ = PendingType::Token;
  return true;
}

bool JsonPullSerializer::prepareFloat(JsonFloat value) {
  if (isnan(value))
    return prepareToken(ARDUINOJSON_ENABLE_NAN ? "NaN" : "null");

#if ARDUINOJSON_ENABLE_INFINITY
  if (isinf(value)) {
    if (value < 0)
      return prepareToken("-Infinity");
    else
      return prepareToken("Infinity");
  }
#else
  if (isinf(value))
    return prepareToken("null");
#endif

  char* p = token_;
  char* end = token_ + kTokenCapacity;

  if (value < 0) {
    *p++ = '-';
    value = -value;
  }

  auto parts = decomposeFloat(value, sizeof(JsonFloat) >= 8 ? 9 : 6);

  {
    char tmp[24];
    char* tmpEnd = tmp + sizeof(tmp);
    size_t n = writeUnsignedToBuffer(tmpEnd, parts.integral);
    if (size_t(end - p) < n)
      return false;
    memcpy(p, tmpEnd - n, n);
    p += n;
  }

  if (parts.decimalPlaces) {
    if (p >= end)
      return false;
    *p++ = '.';

    char tmp[16];
    char* tmpEnd = tmp + sizeof(tmp);
    char* q = tmpEnd;
    uint32_t decimal = parts.decimal;
    int8_t width = parts.decimalPlaces;

    while (width--) {
      *--q = char(decimal % 10 + '0');
      decimal /= 10;
    }

    size_t n = size_t(tmpEnd - q);
    if (size_t(end - p) < n)
      return false;
    memcpy(p, q, n);
    p += n;
  }

  if (parts.exponent) {
    if (p >= end)
      return false;
    *p++ = 'e';

    char tmp[16];
    char* tmpEnd = tmp + sizeof(tmp);
    size_t n = writeSignedToBuffer(tmpEnd, parts.exponent);
    if (size_t(end - p) < n)
      return false;
    memcpy(p, tmpEnd - n, n);
    p += n;
  }

  tokenBegin_ = 0;
  tokenEnd_ = size_t(p - token_);
  pendingType_ = PendingType::Token;
  return true;
}

bool JsonPullSerializer::prepareBool(bool value) {
  return prepareToken(value ? "true" : "false");
}

bool JsonPullSerializer::prepareNull() {
  return prepareToken("null");
}

bool JsonPullSerializer::prepareRaw(RawString value) {
  rawPtr_ = value.data();
  rawSize_ = value.size();
  rawIndex_ = 0;
  pendingType_ = PendingType::Raw;
  return true;
}

bool JsonPullSerializer::prepareString(const char* s) {
  return prepareString(s, strlen(s));
}

bool JsonPullSerializer::prepareString(const char* s, size_t n) {
  pendingString_.ptr = s;
  pendingString_.size = n;
  pendingString_.index = 0;
  pendingString_.quoteOpenPending = true;
  pendingString_.quoteClosePending = false;
  pendingString_.escapeLen = 0;
  pendingString_.escapePos = 0;
  pendingType_ = PendingType::String;
  return true;
}

bool JsonPullSerializer::nextEscape(char c) {
  char specialChar = EscapeSequence::escapeChar(c);
  if (specialChar) {
    pendingString_.escape[0] = '\\';
    pendingString_.escape[1] = specialChar;
    pendingString_.escapeLen = 2;
    pendingString_.escapePos = 0;
    return true;
  } else if (c == 0) {
    memcpy(pendingString_.escape, "\\u0000", 6);
    pendingString_.escapeLen = 6;
    pendingString_.escapePos = 0;
    return true;
  } else {
    pendingString_.escape[0] = c;
    pendingString_.escapeLen = 1;
    pendingString_.escapePos = 0;
    return true;
  }
}

size_t JsonPullSerializer::drainPendingToken(uint8_t* output, size_t capacity) {
  size_t n = tokenEnd_ - tokenBegin_;
  if (n > capacity)
    n = capacity;
  memcpy(output, token_ + tokenBegin_, n);
  tokenBegin_ += n;
  if (tokenBegin_ == tokenEnd_) {
    tokenBegin_ = 0;
    tokenEnd_ = 0;
    pendingType_ = PendingType::None;
  }
  return n;
}

size_t JsonPullSerializer::drainPendingRaw(uint8_t* output, size_t capacity) {
  size_t n = rawSize_ - rawIndex_;
  if (n > capacity)
    n = capacity;
  memcpy(output, rawPtr_ + rawIndex_, n);
  rawIndex_ += n;
  if (rawIndex_ == rawSize_) {
    rawPtr_ = 0;
    rawSize_ = 0;
    rawIndex_ = 0;
    pendingType_ = PendingType::None;
  }
  return n;
}

size_t JsonPullSerializer::drainPendingString(uint8_t* output, size_t capacity) {
  size_t written = 0;

  while (written < capacity) {
    if (pendingString_.quoteOpenPending) {
      output[written++] = '"';
      pendingString_.quoteOpenPending = false;
      continue;
    }

    if (pendingString_.escapePos < pendingString_.escapeLen) {
      output[written++] =
          uint8_t(pendingString_.escape[pendingString_.escapePos++]);
      if (pendingString_.escapePos == pendingString_.escapeLen) {
        pendingString_.escapeLen = 0;
        pendingString_.escapePos = 0;
      }
      continue;
    }

    if (pendingString_.index < pendingString_.size) {
      nextEscape(pendingString_.ptr[pendingString_.index++]);
      continue;
    }

    if (!pendingString_.quoteClosePending) {
      pendingString_.quoteClosePending = true;
      continue;
    }

    output[written++] = '"';
    pendingString_.quoteClosePending = false;
    pendingType_ = PendingType::None;
    break;
  }

  return written;
}

size_t JsonPullSerializer::drainPending(uint8_t* output, size_t capacity) {
  switch (pendingType_) {
    case PendingType::Token:
      return drainPendingToken(output, capacity);
    case PendingType::String:
      return drainPendingString(output, capacity);
    case PendingType::Raw:
      return drainPendingRaw(output, capacity);
    case PendingType::None:
    default:
      return 0;
  }
}

bool JsonPullSerializer::beginValue(VariantData* data) {
  if (!data)
    return prepareNull();

  if (data->isArray()) {
    Frame& f = stack_[depth_ - 1];
    f.type = FrameType::Array;
    f.slotId = data->content.asCollection.head;
    f.started = false;
    f.afterChild = false;
    f.isKey = false;
    return true;
  }

  if (data->isObject()) {
    Frame& f = stack_[depth_ - 1];
    f.type = FrameType::Object;
    f.slotId = data->content.asCollection.head;
    f.started = false;
    f.afterChild = false;
    f.isKey = true;
    return true;
  }

  JsonVariantConst value(data, resources_);

  if (value.is<JsonString>()) {
    JsonString s = value.as<JsonString>();
    return prepareString(s.c_str(), s.size());
  }

  if (value.is<RawString>()) {
    return prepareRaw(value.as<RawString>());
  }

  if (value.is<JsonInteger>()) {
    return prepareInteger(value.as<JsonInteger>());
  }

  if (value.is<JsonUInt>()) {
    return prepareInteger(value.as<JsonUInt>());
  }

  if (value.is<bool>()) {
    return prepareBool(value.as<bool>());
  }

  if (value.is<JsonFloat>()) {
    return prepareFloat(value.as<JsonFloat>());
  }

  if (value.is<const char*>()) {
    return prepareString(value.as<const char*>());
  }

  if (value.isNull())
    return prepareNull();

  return prepareNull();
}

size_t JsonPullSerializer::read(uint8_t* output, size_t capacity) {
  if (capacity == 0)
    return 0;

  size_t written = 0;

  if (peeked_ >= 0) {
    output[written++] = uint8_t(peeked_);
    peeked_ = -1;
    if (written == capacity)
      return written;
  }

  while (written < capacity) {
    if (pendingType_ != PendingType::None) {
      written += drainPending(output + written, capacity - written);
      if (written == capacity)
        break;
      continue;
    }

    if (finished_)
      break;

    if (!rootPushed_) {
      rootPushed_ = true;
      if (!pushValue(root_)) {
        finished_ = true;
        break;
      }
      continue;
    }

    if (depth_ == 0) {
      finished_ = true;
      break;
    }

    Frame& frame = stack_[depth_ - 1];

    if (frame.type == FrameType::Value) {
      if (!frame.started) {
        frame.started = true;
        if (!beginValue(frame.data)) {
          finished_ = true;
          break;
        }

        if (pendingType_ == PendingType::None &&
            frame.type == FrameType::Value) {
          popFrame();
        }
      } else if (pendingType_ == PendingType::None) {
        popFrame();
      }
      continue;
    }

    if (frame.type == FrameType::Array) {
      if (!frame.started) {
        frame.started = true;
        emitSingleChar('[');
        if (frame.slotId == NULL_SLOT)
          frame.afterChild = true;
        continue;
      }

      if (frame.afterChild) {
        if (frame.slotId != NULL_SLOT) {
          emitSingleChar(',');
          frame.afterChild = false;
          continue;
        } else {
          emitSingleChar(']');
          popFrame();
          continue;
        }
      }

      if (frame.slotId == NULL_SLOT) {
        emitSingleChar(']');
        popFrame();
        continue;
      }

      VariantData* child = resources_->getVariant(frame.slotId);
      frame.slotId = child->next;
      frame.afterChild = true;
      pushValue(child);
      continue;
    }

    if (frame.type == FrameType::Object) {
      if (!frame.started) {
        frame.started = true;
        emitSingleChar('{');
        if (frame.slotId == NULL_SLOT)
          frame.afterChild = true;
        continue;
      }

      if (frame.afterChild) {
        if (frame.slotId != NULL_SLOT) {
          emitSingleChar(',');
          frame.afterChild = false;
          frame.isKey = true;
          continue;
        } else {
          emitSingleChar('}');
          popFrame();
          continue;
        }
      }

      if (frame.slotId == NULL_SLOT) {
        emitSingleChar('}');
        popFrame();
        continue;
      }

      VariantData* child = resources_->getVariant(frame.slotId);
      frame.slotId = child->next;

      if (frame.isKey) {
        frame.isKey = false;
        pushValue(child);
        continue;
      } else {
        emitSingleChar(':');
        frame.isKey = true;
        pushValue(child);
        frame.afterChild = true;
        continue;
      }
    }
  }

  return written;
}

ARDUINOJSON_END_PRIVATE_NAMESPACE

namespace HttpServerAdvanced {

JsonSerializerStream::JsonSerializerStream(ArduinoJson::JsonDocument&& doc)
    : serializer_(ArduinoJson::detail::move(doc)) {}

AvailableResult JsonSerializerStream::available() {
  const int availableBytes = serializer_.available();
  if (availableBytes > 0)
    return AvailableBytes(static_cast<size_t>(availableBytes));

  return ExhaustedResult();
}

size_t JsonSerializerStream::read(span<uint8_t> buffer) {
  if (buffer.empty())
    return 0;

  return serializer_.read(buffer.data(), buffer.size());
}

size_t JsonSerializerStream::peek(span<uint8_t> buffer) {
  if (buffer.empty())
    return 0;

  const int value = serializer_.peek();
  if (value < 0)
    return 0;

  buffer[0] = static_cast<uint8_t>(value);
  return 1;
}

}  // namespace HttpServerAdvanced