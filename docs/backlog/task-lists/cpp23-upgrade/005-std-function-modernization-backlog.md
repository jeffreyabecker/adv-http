# std::function Modernization Backlog

_Changelog: 2026-04-15 — evaluated SFNM-05/06 against current call sites and `c:\ode\pled`; recorded callback copyability decisions (GitHub Copilot)_
_Changelog: 2026-04-15 — completed SFNM-04 and SFNM-07 with passing native tests (GitHub Copilot)_
_Changelog: 2026-04-15 — completed SFNM-01/02/03 with move-only callback migration and passing native tests (GitHub Copilot)_
_Changelog: 2026-04-12 — initial draft (GitHub Copilot)_

## Summary

`std::function` is used throughout the library for stored callbacks, type-aliases, and function parameters. C++23 introduces `std::move_only_function` and `std::copyable_function`, which fix two long-standing defects in `std::function`: the requirement that wrapped callables be `CopyConstructible`, and the lack of `const`-qualification propagation from the wrapper to the call operator. C++20 concepts (`std::invocable`) can eliminate type-erasure entirely at call sites where the callable is consumed inline and no storage is needed.

The goals are (1) remove the accidental copyability requirement on stored callbacks that capture move-only state, (2) restore const-correctness, and (3) eliminate heap allocation at call sites where a template parameter is more appropriate.

## Goal / Acceptance Criteria

- Every stored `std::function` member that does not need to be copied is converted to `std::move_only_function`.
- Every public type alias backed by `std::function` that must remain copyable is converted to `std::copyable_function`.
- Inline function parameters in `BasicAuthentication` that only call the callable once are converted to template parameters constrained by `std::invocable`.
- No `std::function` usage remains that does not have a deliberate rationale.
- CTest suite passes after each task.

## Tasks

### SFNM-01 — Convert `IHttpHandler` callback aliases to `std::move_only_function`

Scope: `src/lumalink/http/handlers/IHttpHandler.h`

- [x] Change `IHttpHandler::Callback` (`std::function<HandlerResult()>`) to `std::move_only_function<HandlerResult()>`
- [x] Change `IHttpHandler::InvocationCallback` to `std::move_only_function<HandlerResult(HttpRequestContext &)>`
- [x] Change `IHttpHandler::InterceptorCallback` to `std::move_only_function<HandlerResult(HttpRequestContext &, InvocationNext)>`
- [x] Change `IHttpHandler::Predicate` to `std::move_only_function<bool(HttpRequestContext &)>`
- [x] Change `IHttpHandler::Factory` to `std::move_only_function<std::unique_ptr<IHttpHandler>(HttpRequestContext &)>`
- [x] Update `#include` directives (add `<functional>` guard if not already present; `std::move_only_function` lives there)
- [x] Fix any call sites that attempted to copy these types (should be zero; confirm by building)

**Note:** `InvocationNext` is itself one of these aliases; convert all in one pass to avoid transient inconsistency in the virtual dispatch chain.

### SFNM-02 — Convert `HttpServerBase::pipelineHandlerFactory_` to `std::move_only_function`

Scope: `src/lumalink/http/server/HttpServerBase.h`, `HttpServerBase.cpp`

- [x] Change `pipelineHandlerFactory_` member from `std::function<PipelineHandlerPtr(HttpServerBase &)>` to `std::move_only_function<PipelineHandlerPtr(HttpServerBase &)>`
- [x] Change the `setPipelineHandlerFactory` parameter type to match
- [x] Confirm no copy of the factory is made anywhere in the server body

### SFNM-03 — Convert `HandlerBuilder::addHandler_` to `std::move_only_function`

Scope: `src/lumalink/http/routing/HandlerBuilder.h`

- [x] Change the `addHandler_` data member to `std::move_only_function<void(IHttpHandler::Predicate, IHttpHandler::Factory)>`
- [x] Change both constructor parameter types to match (they are `std::move`d into the member already)
- [x] Confirm the destructor fires the callback exactly once and no copy is attempted

### SFNM-04 — Convert `BufferedStringBodyHandler::handler_` to `std::move_only_function`

Scope: `src/lumalink/http/handlers/BufferedStringBodyHandler.h`, `BufferedStringBodyHandler.cpp`

- [x] Change `handler_` and the `Invocation`/`InvocationWithoutParams` type aliases to `std::move_only_function`
- [x] Update constructor parameters and `makeFactory`/`curryInterceptor`/`applyFilter` signatures accordingly

### SFNM-05 — Evaluate `WebSocketCallbacks` members for `std::copyable_function`

Scope: `src/lumalink/http/websocket/WebSocketCallbacks.h`

- [x] Determine whether `WebSocketCallbacks` structs are ever copied at call sites (e.g., passed by value, stored in containers that copy elements)
- [x] If no copy is required: convert `onOpen`, `onText`, `onBinary`, `onClose`, `onError` to `std::move_only_function`
- [x] If copies are required: convert to `std::copyable_function` (const-correct, still copyable)
- [x] Update any `= nullptr` default initialisers — both `std::move_only_function` and `std::copyable_function` support default-construction to empty

Decision: `WebSocketCallbacks` should remain copyable and migrate to `std::copyable_function`, not `std::move_only_function`.

Rationale:
- Current library code already treats the aggregate as a copyable by-value object: `ProviderRegistryBuilder::websocket(std::string_view, WebSocketCallbacks)` accepts callbacks by value and existing routing tests pass an lvalue callbacks struct without `std::move`.
- `WebSocketContext` stores a by-value `WebSocketCallbacks` member for session lifetime, so preserving value semantics avoids pushing move-only requirements through the upgrade path API.
- No uses of `WebSocketCallbacks`, `CrossOriginRequestSharing`, or `DefaultFileLocator` were found in `c:\ode\pled`, so there is no external consumer evidence pushing toward a more restrictive move-only contract here.

### SFNM-06 — Evaluate public type aliases `CorsRequestFilter` and `RequestPathPredicate`

Scope: `src/lumalink/http/routing/CrossOriginRequestSharing.h`, `src/lumalink/http/staticfiles/DefaultFileLocator.h`

- [x] Audit whether consumers ever copy values of these alias types
- [x] Convert to `std::move_only_function` or `std::copyable_function` as appropriate
- [x] If conversion would constitute a breaking API change in an already-published context, note rationale

Decision:
- `CorsRequestFilter`: convert to `std::move_only_function`
- `RequestPathPredicate`: convert to `std::move_only_function`

Rationale:
- No consumer usages of either alias were found in `c:\ode\pled`.
- In the current library, `CorsRequestFilter` is only constructed from temporaries and moved into the CORS component path; the remaining copy requirement is an implementation detail inside `CreateCorsHandlingComponent`, not a demonstrated public semantic requirement.
- `RequestPathPredicate` is stored once in `DefaultFileLocator` and moved through `StaticFilesBuilder`'s deferred setup path; no current tests or external consumers require copying predicates after creation.
- Converting these public aliases to move-only wrappers is still a source-level contract change for callers that rely on implicit lvalue copies. Because no such consumer was found in `pled`, the change looks acceptable for this codebase, but it should still be called out in release notes when implemented.

### SFNM-07 — Replace `std::function` parameters in `BasicAuthentication` with `std::invocable` templates

Scope: `src/lumalink/http/routing/BasicAuthentication.h`

- [x] Convert the `validator` and `onSuccess` parameters of `CheckBasicAuthCredentials` to template parameters constrained by `std::invocable<std::string_view, std::string_view>`
- [x] Convert the same parameters in `BasicAuth` factory functions where the callable is consumed inline and not stored beyond the call
- [x] Keep `onFailure` as `std::move_only_function` (or `std::function` + default arg) because it has a default value and the template approach requires a different defaulting strategy
- [x] Add `#include <concepts>` where needed; remove `#include <functional>` if no other use remains in the header

## Owner

Unassigned

## Priority

Low — quality / correctness improvement; no functional change

## References

- `src/lumalink/http/handlers/IHttpHandler.h`
- `src/lumalink/http/handlers/BufferedStringBodyHandler.h`
- `src/lumalink/http/server/HttpServerBase.h`
- `src/lumalink/http/routing/HandlerBuilder.h`
- `src/lumalink/http/routing/BasicAuthentication.h`
- `src/lumalink/http/routing/CrossOriginRequestSharing.h`
- `src/lumalink/http/staticfiles/DefaultFileLocator.h`
- `src/lumalink/http/websocket/WebSocketCallbacks.h`
- cppreference: [std::move_only_function](https://en.cppreference.com/w/cpp/utility/functional/move_only_function), [std::copyable_function](https://en.cppreference.com/w/cpp/utility/functional/copyable_function), [std::invocable](https://en.cppreference.com/w/cpp/concepts/invocable)
