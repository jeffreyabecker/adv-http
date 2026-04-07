## C++ Rules

- Use C++17 features as supported by the project toolchain.
- Follow Allman braces and 4-space indentation.
- Use `#pragma once` in headers.
- Prefer `std::optional`, `std::variant`, `std::array`, smart pointers, and `constexpr`.
- Avoid exceptions and RTTI-heavy patterns unless explicitly requested.
- Do not use global mutable state when avoidable.

## API Evolution

- The library does not currently have a stable API. Prefer clean internal and public API improvements over preserving backward compatibility.
- If an API, type, method, header, or behavior should be removed or changed, remove or change it entirely rather than keeping deprecated paths alive.
- Do not add shim headers, compatibility wrappers, alias types, forwarding overloads, or compatibility helper functions solely to preserve older call sites.
- Do not update examples to preserve or demonstrate transitional APIs unless explicitly requested.

## Compiler Constants and Macros

- Prefer `static constexpr` constants in C++ code over direct macro usage whenever possible.
- Use macros primarily as compile-time override hooks for PlatformIO/build flags.
- Define every overrideable compiler constant using this pattern:

```cpp
#ifndef LUMALINK_HTTP_[COMPILER_CONSTANT]
#define LUMALINK_HTTP_[COMPILER_CONSTANT] (default value)
#endif
static constexpr <type> [CompilerConstant] = LUMALINK_HTTP_[COMPILER_CONSTANT];
```

- Code should read/use the `static constexpr` variable (`[CompilerConstant]`) rather than the raw `LUMALINK_HTTP_*` macro, except where preprocessor logic is explicitly required (`#if`, `#ifdef`).
- Keep macro names `UPPER_SNAKE_CASE` with `LUMALINK_HTTP_` prefix; keep constexpr names `PascalCase` or existing project style for constants.

## Backlog / Task-List (concise)

- Filenames: use a three-digit, zero-padded numeric prefix + short-hyphen-title + `-backlog.md`.
- Pick the smallest unused numeric prefix in `docs/backlog/task-lists` when adding a new item.
- Minimal template: Title, short Summary (2–4 sentences), Goal/Acceptance Criteria, Tasks (checklist), Owner, Priority, References.
- Keep items focused; split large efforts into multiple backlog files.
- After successfully completing work covered by a backlog/task-list item, update that file before finishing: mark completed checklist items, adjust status/notes to match the new state, and add the changelog note for the update.
- When updating a backlog file, add a one-line changelog note (date + author) at the top.

(For automation, run a quick directory listing to find the next free prefix.)
