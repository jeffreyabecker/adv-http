## C++ Rules

- Use C++17 features as supported by the project toolchain.
- Follow Allman braces and 4-space indentation.
- Use `#pragma once` in headers.
- Prefer `std::optional`, `std::variant`, `std::array`, smart pointers, and `constexpr`.
- Avoid exceptions and RTTI-heavy patterns unless explicitly requested.
- Do not use global mutable state when avoidable.

## Compiler Constants and Macros

- Prefer `static constexpr` constants in C++ code over direct macro usage whenever possible.
- Use macros primarily as compile-time override hooks for PlatformIO/build flags.
- Define every overrideable compiler constant using this pattern:

```cpp
#ifndef HTTPSERVER_ADVANCED_[COMPILER_CONSTANT]
#define HTTPSERVER_ADVANCED_[COMPILER_CONSTANT] (default value)
#endif
static constexpr <type> [CompilerConstant] = HTTPSERVER_ADVANCED_[COMPILER_CONSTANT];
```

- Code should read/use the `static constexpr` variable (`[CompilerConstant]`) rather than the raw `HTTPSERVER_ADVANCED_*` macro, except where preprocessor logic is explicitly required (`#if`, `#ifdef`).
- Keep macro names `UPPER_SNAKE_CASE` with `HTTPSERVER_ADVANCED_` prefix; keep constexpr names `PascalCase` or existing project style for constants.

## Backlog / Task-List (concise)

- Filenames: use a three-digit, zero-padded numeric prefix + short-hyphen-title + `-backlog.md`.
- Pick the smallest unused numeric prefix in `docs/backlog/task-lists` when adding a new item.
- Minimal template: Title, short Summary (2–4 sentences), Goal/Acceptance Criteria, Tasks (checklist), Owner, Priority, References.
- Keep items focused; split large efforts into multiple backlog files.
- When updating a backlog file, add a one-line changelog note (date + author) at the top.

(For automation, run a quick directory listing to find the next free prefix.)
