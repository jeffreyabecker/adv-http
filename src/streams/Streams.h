#pragma once
#include "ByteStream.h"

namespace HttpServerAdvanced
{
    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };
}
