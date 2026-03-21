#pragma once

namespace HttpServerAdvanced
{
    namespace Compat
    {
        // COMPAT-005 will replace this forward declaration with the concrete
        // clock abstraction used by timeout and activity tracking code.
        class Clock;
    }
}