#pragma once
#include "../compat/ByteStream.h"

namespace httpadv::v1::streams
{
    using httpadv::v1::transport::SingleByteSource;

    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };
}
