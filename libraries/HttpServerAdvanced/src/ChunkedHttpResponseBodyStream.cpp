#include "./ChunkedHttpResponseBodyStream.h"
#include <cstdio>
#include <cstdint>

namespace HttpServerAdvanced {

void ChunkedHttpResponseBodyStream::consume() {
    if (length_ > 0) {
        --length_;
        ++head_;
        if (length_ == 0) {
            head_ = 0;
        }
    }
}

constexpr std::size_t ChunkedHttpResponseBodyStream::hex_length(std::uint64_t n) {
    std::size_t len = 1;
    while (n >= 0x10) {
        n /= 0x10;
        ++len;
    }
    return len;
}

void ChunkedHttpResponseBodyStream::bufferNextChunk() {
    if (haveBufferedTerminalChunk_) {
        return;
    }

    size_t dataOffset = hex_length(HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE) + 3;
    size_t maxInnerReadLength = HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE - dataOffset - 2;
    size_t bytesRead = 0;
    while (bytesRead < maxInnerReadLength) {
        int byte = innerStream_->read();
        if (byte == -1) {
            break;
        }
        buffer_[bytesRead] = static_cast<uint8_t>(byte);
        ++bytesRead;
    }
    if (bytesRead == 0) {
        for (size_t i = 0; i < finalChunkLength; i++) {
            buffer_[i] = finalChunk[i];
        }
        length_ = finalChunkLength;
        head_ = 0;
        haveBufferedTerminalChunk_ = true;
        return;
    } else {
        auto printed = std::snprintf(reinterpret_cast<char *>(buffer_.data()), dataOffset, "%zx\r\n", bytesRead);
        auto toRemove = dataOffset - static_cast<size_t>(printed);
        if (toRemove > 0) {
            for (size_t i = 0; i < bytesRead; ++i) {
                buffer_[printed + i] = buffer_[dataOffset + i];
            }
            length_ = static_cast<size_t>(printed) + bytesRead + 2;
            head_ = 0;
        } else {
            length_ = dataOffset + bytesRead + 2;
            head_ = 0;
        }
    }
}

ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<Stream> innerStream)
    : HttpResponseBodyStream(std::move(innerStream)) {}

int ChunkedHttpResponseBodyStream::available() {
    if (done_) {
        return 0;  // ✅ End of stream
    }
    if (length_ > 0) {
        return static_cast<int>(length_);  // ✅ Data available in buffer
    }
    // Buffer empty, try to fill it
    bufferNextChunk();
    if (length_ > 0) {
        return static_cast<int>(length_);  // ✅ Data available after buffering
    }
    if (haveBufferedTerminalChunk_) {
        done_ = true;
        return 0;  // ✅ Terminal chunk buffered, end of stream
    }
    return -1;  // ✅ Buffer empty, more data expected from innerStream_
}

int ChunkedHttpResponseBodyStream::read() {
    int result = peek();
    consume();
    return result;
}

int ChunkedHttpResponseBodyStream::peek() {
    if (done_) {
        return -1;
    }
    if (length_ == 0) {
        bufferNextChunk();
    }
    if (length_ == 0) {
        done_ = true;
        return -1;
    }
    return buffer_[head_];
}

size_t ChunkedHttpResponseBodyStream::write(uint8_t b) {
    (void)b;
    return 0;
}

std::unique_ptr<HttpResponseBodyStream> ChunkedHttpResponseBodyStream::create(std::unique_ptr<Stream> innerStream) {
    return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerStream));
}

} // namespace HttpServerAdvanced
