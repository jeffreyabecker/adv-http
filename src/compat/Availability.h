#pragma once

#include <cstddef>

namespace HttpServerAdvanced {

enum class AvailabilityState { HasBytes, Exhausted, TemporarilyUnavailable, Error };

struct AvailableResult {
    AvailabilityState state;
    std::size_t count;
    int errorCode;
};

} // namespace HttpServerAdvanced
