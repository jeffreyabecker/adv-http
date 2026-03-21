// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

#if defined(__has_include)
#  if __has_include(<span>)
#    include <span>
#    define HSA_USE_STD_SPAN 1
#  elif __has_include("span.hpp")
#    include "span.hpp"
#    define HSA_USE_TCB_SPAN 1
#  endif
#endif

namespace HttpServerAdvanced {

#if defined(HSA_USE_STD_SPAN)
template <typename T>
using span = std::span<T>;

template <typename T>
using byte_span = std::span<T>;

#elif defined(HSA_USE_TCB_SPAN)
template <typename T>
using span = TCB_SPAN_NAMESPACE_NAME::span<T>;

template <typename T>
using byte_span = TCB_SPAN_NAMESPACE_NAME::span<T>;

#else
#error "HttpServerAdvanced::span requires either a C++20 <span> or the compat/span.hpp (tcbrindle) implementation. Add src/compat/span.hpp or enable C++20 support."

#endif // HSA_USE_STD_SPAN / HSA_USE_TCB_SPAN

} // namespace HttpServerAdvanced
