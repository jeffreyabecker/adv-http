# LumaLink HTTP Rename Release Notes

## Summary

This release is a strict breaking rename from the historical `HttpServerAdvanced` / `httpadv::v1` surface to the final `lumalink::http` surface. The cutover is direct: no compatibility headers, namespace aliases, wrapper APIs, or fallback include paths are provided.

## Final Public Surface

- Package metadata publishes the library as `LumaLink HTTP` / `lumalink-http`.
- The canonical umbrella header is `#include <lumalink/http/LumaLinkHttp.h>`.
- Public C++ namespaces are rooted at `lumalink::http`.
- Platform implementations and factories remain owned by `lumalink::platform` and must be included directly from `lumalink/platform/...`.

## Required Consumer Changes

1. Replace legacy umbrella includes with `#include <lumalink/http/LumaLinkHttp.h>`.
2. Replace `httpadv::v1` and `httpadv` usage with `lumalink::http`.
3. Include concrete platform factories from `lumalink/platform/...` where server construction depends on platform transport implementations.
4. Update package metadata expectations for Arduino and PlatformIO consumers to the new LumaLink naming.

## Unsupported Legacy Paths

- `HttpServerAdvanced.h`
- `httpadv/...`
- `httpadv::v1`
- namespace alias bridges or compatibility wrapper headers

## Validation Status

- Native regression suite passes on the renamed umbrella/header surface.
- Repository checks fail if legacy identifiers are reintroduced outside approved historical planning and inventory documents.

## Migration Example

```cpp
#include <lumalink/http/LumaLinkHttp.h>
#include <lumalink/platform/arduino/ArduinoWiFiTransport.h>

using namespace lumalink::http::core;
using namespace lumalink::http::handlers;
using namespace lumalink::http::response;
using namespace lumalink::http::server;

WebServer server(lumalink::platform::arduino::ArduinoWiFiTransportFactory::createServer(80));
```