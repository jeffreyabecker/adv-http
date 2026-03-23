#pragma once

#include "Clock.h"
#include "IFileSystem.h"
#include "FileSystem.h"
#include "Stream.h"

namespace HttpServerAdvanced
{
    namespace Compat
    {
        // Canonical home for Arduino alias-or-shim types.
        //
        // Boundary rules:
        // - Core headers should depend on compatibility headers rather than
        //   including Arduino headers directly.
        // - Compatibility leaf headers own the ARDUINO/non-ARDUINO split.
        // - Arduino adapters may still include Arduino headers directly when
        //   bridging to framework types.
    }
}