#pragma once
#include "../compat/ByteStream.h"

namespace HttpServerAdvanced
{
    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };
}
