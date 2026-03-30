#pragma once

#include "Clock.h"
#include "../transport/IFileSystem.h"

namespace httpadv::v1::compat
{
    // Canonical home for the supported compatibility seams.
    //
    // Boundary rules:
    // - Core headers should depend on compatibility headers rather than
    //   including Arduino headers directly.
    // - Compatibility leaf headers own the ARDUINO/non-ARDUINO split.
    // - Arduino adapters may still include Arduino headers directly when
    //   bridging to framework types.
}