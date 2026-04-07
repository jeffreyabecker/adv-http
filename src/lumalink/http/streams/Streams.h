#pragma once
#include <exception>
#include "LumaLinkPlatform.h"

namespace lumalink::http::streams
{
    using lumalink::platform::buffers::SingleByteSource;

    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };
}
