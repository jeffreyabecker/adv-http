#pragma once

#include "../../IFileSystem.h"

#include <memory>
#include <string_view>

#if defined(ARDUINO) && __has_include(<FS.h>)
#include <FS.h>
#include "ArduinoFileAdapterImpl.h"

namespace HttpServerAdvanced
{
} // namespace HttpServerAdvanced

#else

namespace HttpServerAdvanced
{
    class FS;

} // namespace HttpServerAdvanced

#endif