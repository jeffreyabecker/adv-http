# HttpServerAdvanced Examples Plan

Examples are organized by complexity and feature coverage. Each example builds on concepts from previous ones and is intended to keep the HTTP application layer portable across embedded and desktop targets.

Planned examples should isolate target bootstrapping from request handling so the same routing, handler, response, and static-file logic can be exercised in firmware builds and native host-side runs. JSON examples assume optional JSON support is enabled. HTTPS/TLS server examples are intentionally out of scope for this repository.

---

## 1. Getting Started

### 1.1 HelloWorld
**File**: `examples/HelloWorld/main.cpp`  
**Features**: Basic server setup, single GET endpoint, text response  
**Concepts**:
- `WebServer` instantiation
- application bootstrap separated from route wiring
- `GetRequest` handler
- `StringResponse::create()` usage

```
GET /         → "Hello, World!"
GET /status   → JSON status response
```

---

### 1.2 MultipleRoutes
**File**: `examples/MultipleRoutes/main.cpp`  
**Features**: Multiple endpoints, different response types  
**Concepts**:
- Multiple `.on<GetRequest>()` registrations
- `HttpContentTypes` usage
- `HttpHeader::ContentType()`
- Custom status codes

```
GET /          → HTML welcome page
GET /api/time  → JSON with current timestamp
GET /api/health → JSON with runtime health data
```

---
### 1.3 StaticFiles
**File**: `examples/StaticFiles/main.cpp`  
**Features**: Serving files from filesystem  
**Concepts**:
- `StaticFiles()` builder
- filesystem adapter setup
- Path prefixes and content root
- MIME type auto-detection

```
GET /index.html   → Serve /wwwroot/index.html
GET /css/app.css  → Serve /wwwroot/css/app.css
GET /api/data     → API route (not static)
```

---

## 2. Request Data

### 2.1 UrlParameters
**File**: `examples/UrlParameters/main.cpp`  
**Features**: Parameterized routes, extracting URL segments  
**Concepts**:
- Wildcard routes: `/api/users/?` (`?` is the default value of `REQUEST_MATCHER_PATH_WILDCARD_CHAR`)
- Accessing `params` vector
- Multiple parameters: `/api/?/items/?`

```
GET /api/users/123     → User 123 details
GET /led/on            → Turn LED on
GET /led/off           → Turn LED off
GET /gpio/5/set/1      → Set GPIO 5 HIGH
```

---

### 2.2 QueryStrings
**File**: `examples/QueryStrings/main.cpp`  
**Features**: Parsing query string parameters  
**Concepts**:
- `HttpContext.uri()` access
- `HttpUtility::ParseQueryString()`
- `KeyValuePairView` iteration

```
GET /search?q=hello&limit=10  → Parse and echo params
GET /config?wifi=ssid&pass=pw → Configuration endpoint
```

---

### 2.3 FormParsing
**File**: `examples/FormParsing/main.cpp`  
**Features**: POST form data handling  
**Concepts**:
- `Form` handler type
- `PostBodyData` (KeyValuePairView)
- Automatic content-type restriction

```
POST /login    → Username/password form
POST /settings → Key-value configuration
```

---

### 2.4 JsonApi
**File**: `examples/JsonApi/main.cpp`  
**Features**: JSON request/response API  
**Concepts**:
- `Json` handler type
- `JsonDocument` access when optional JSON support is enabled
- JSON response building

```
POST /api/config  → Receive JSON, update config
GET  /api/config  → Return current config as JSON
POST /api/led     → {"state": true} to control LED
```

---

## 3. Security

### 3.1 BasicAuthentication
**File**: `examples/BasicAuthentication/main.cpp`  
**Features**: HTTP Basic Auth protection  
**Concepts**:
- `BasicAuth()` interceptor
- Credential validation callback
- Protected vs public routes
- `.with()` interceptor chaining

```
GET /public   → No auth required
GET /private  → Requires Basic Auth
GET /admin    → Different credentials
```

---

### 3.2 CorsSupport
**File**: `examples/CorsSupport/main.cpp`  
**Features**: Cross-Origin Resource Sharing  
**Concepts**:
- `CrossOriginRequestSharing()` response filter
- `.apply()` chaining
- Preflight OPTIONS handling
- Allow-Origin, Allow-Methods, Allow-Headers

```
OPTIONS /api/data  → CORS preflight
POST    /api/data  → With CORS headers
```

---

## 4. Advanced Patterns

### 4.1 InterceptorPipeline
**File**: `examples/InterceptorPipeline/main.cpp`  
**Features**: Custom middleware/interceptors  
**Concepts**:
- Writing custom `InterceptorCallback`
- Request logging
- Response timing
- Error handling middleware
- Chaining multiple interceptors

```
Logging → Auth → Handler → CORS → Response
```

---

### 4.2 ResponseFilters
**File**: `examples/ResponseFilters/main.cpp`  
**Features**: Response modification  
**Concepts**:
- Custom `ResponseFilter` functions
- Adding default headers
- Response compression (future)
- ETag generation

```
All responses get Server header, timing header
```

---

### 4.3 RawBodyStreaming
**File**: `examples/RawBodyStreaming/main.cpp`  
**Features**: Streaming large request bodies  
**Concepts**:
- `RawBody` handler type
- `RawBodyBuffer` chunk handling
- Content-Length tracking
- Memory-efficient large POSTs

```
POST /firmware  → OTA update endpoint
POST /bulk      → Large data ingestion
```

---

### 4.4 CustomHandlerMatcher
**File**: `examples/CustomHandlerMatcher/main.cpp`  
**Features**: Advanced route matching  
**Concepts**:
- Custom `MethodChecker`
- Custom `UriPatternChecker`
- Custom `ContentTypeChecker`
- Regex-like matching (manual)

```
Match /api/v{1,2}/users with custom logic
```

---

### 4.5 ErrorHandling
**File**: `examples/ErrorHandling/main.cpp`  
**Features**: Error responses and not-found handling  
**Concepts**:
- `onNotFound()` default handler
- HTTP status codes (400, 404, 500)
- Error response formatting
- Exception-safe handlers

```
GET /missing   → Custom 404 page
POST /invalid  → 400 Bad Request
```

---

## 5. Real-World Applications

### 5.1 RestfulApi
**File**: `examples/RestfulApi/main.cpp`  
**Features**: Full REST API example  
**Concepts**:
- GET/POST/PUT/DELETE patterns
- Resource CRUD operations
- JSON input/output
- Proper status codes (200, 201, 204, 404)

```
GET    /api/items      → List items
POST   /api/items      → Create item
GET    /api/items/?    → Get item by ID
PUT    /api/items/?    → Update item
DELETE /api/items/?    → Delete item
```

---

## WebSocket Example Status

- The websocket API is available via `builder.handlers().websocket(path, WebSocketCallbacks)`.
- A minimal echo websocket example is tracked as a follow-up task after the Phase 5 API stabilization work.
- It is intentionally not treated as a merge-gate requirement for core websocket implementation.

---

### 5.2 WebDashboard
**File**: `examples/WebDashboard/main.cpp`  
**Features**: Full web application  
**Concepts**:
- Static files for HTML/CSS/JS
- API endpoints for data
- WebSocket alternative (polling)
- Combining StaticFiles + API

```
/              → Dashboard HTML
/api/sensors   → Sensor data JSON
/api/config    → Configuration API
```

---

### 5.3 OtaUpdate
**File**: `examples/OtaUpdate/main.cpp`  
**Features**: Over-the-air firmware updates  
**Concepts**:
- Multipart or RawBody for firmware binary
- Platform-owned firmware update hook or bootloader integration
- Progress tracking
- Verification and reboot

```
POST /update  → Upload firmware.bin
GET  /version → Current firmware version
```

---

### 6.4 CaptivePortal
**File**: `examples/CaptivePortal/main.cpp`  
**Features**: Wireless provisioning portal  
**Concepts**:
- AP mode configuration
- DNS server for captive portal
- Network scanning and connection
- Credential storage

```
Any request → Redirect to /setup
/setup      → Network configuration page
/connect    → Attempt network connection
```

---

## Example Priority Order

| Priority | Example | Complexity | Key Learning |
|----------|---------|------------|--------------|
| 1 | HelloWorld | ⭐ | Basic setup |
| 2 | MultipleRoutes | ⭐ | Route organization |
| 3 | UrlParameters | ⭐⭐ | Dynamic routes |
| 4 | StaticFiles | ⭐⭐ | File serving |
| 5 | FormParsing | ⭐⭐ | POST handling |
| 6 | JsonApi | ⭐⭐ | JSON API pattern |
| 7 | BasicAuthentication | ⭐⭐ | Security basics |
| 8 | FileUpload | ⭐⭐⭐ | Multipart/streaming |
| 9 | CorsSupport | ⭐⭐ | Cross-origin |
| 10 | RestfulApi | ⭐⭐⭐ | Full API pattern |
| 11 | ErrorHandling | ⭐⭐ | Robust handlers |
| 12 | WebDashboard | ⭐⭐⭐ | Full application |
| 13 | InterceptorPipeline | ⭐⭐⭐ | Advanced middleware |
| 14 | OtaUpdate | ⭐⭐⭐⭐ | Production feature |
| 15 | CaptivePortal | ⭐⭐⭐⭐ | Advanced networking |

---

## File Structure Template

Each example should include:
```
examples/
  ExampleName/
    main.cpp             # Target bootstrap plus application wiring
    README.md            # Example description, environment notes, expected behavior
    fs/                  # Filesystem data (if needed)
      wwwroot/
        index.html
        style.css
```

---

*Generated on 2026-01-22*
