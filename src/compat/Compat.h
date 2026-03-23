#pragma once

#include "Clock.h"
#include "IFileSystem.h"

#ifdef ARDUINO
#include "ArduinoFileSystemAdapter.h"
#endif

namespace HttpServerAdvanced
{
    namespace Compat
    {
        // Canonical home for the supported compatibility seams.
        //
        // Boundary rules:
        // - Core headers should depend on compatibility headers rather than
        //   including Arduino headers directly.
        // - Compatibility leaf headers own the ARDUINO/non-ARDUINO split.
        // - Arduino adapters may still include Arduino headers directly when
        //   bridging to framework types.
        // - Legacy Arduino-shaped shims such as compat/FileSystem.h and
        //   compat/Stream.h are opt-in compatibility headers, not part of
        //   the default umbrella surface.
    }
}