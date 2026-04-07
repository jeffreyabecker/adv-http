#pragma once
#include "../transport/ByteStream.h"

namespace httpadv::v1::streams
{
    using lumalink::platform::buffers::SingleByteSource;

    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };
}
