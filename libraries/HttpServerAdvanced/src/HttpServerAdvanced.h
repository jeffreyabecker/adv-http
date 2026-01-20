// dont do a pragma here because its a wrapper around other include files
// #pragma once

 //TODO: This is the root include of the library. It should include all public headers.
 // Add includes for all public headers here.
 // Add using statements to pull members out of namespaces .
 #include "./StandardHttpServer.h"
 #include "./SecureHttpServer.h"
 #include "./core/Core.h"
 #include "./static/StaticFiles.h"
 #include "./HttpUtility.h"
 #include "./util/UriView.h"
 

using HttpServerAdvanced::HttpServer;
using HttpServerAdvanced::SecureHttpServer;
using HttpServerAdvanced::SecureHttpServerConfig;
using HttpServerAdvanced::Core::CoreServices;
using WebServerBuilder = HttpServerAdvanced::Core::CoreServicesBuilder;
using HttpServerAdvanced::Core::HttpContext;
using HttpServerAdvanced::Core::HttpHandler;
using HttpServerAdvanced::Core::HttpRequest;
using HttpServerAdvanced::Core::HttpResponse;
using HttpServerAdvanced::Core::IHttpResponse;
using HttpServerAdvanced::Core::HttpStatus;
using HttpServerAdvanced::Core::HttpMethod;
using HttpServerAdvanced::Core::HttpHeader;
using HttpServerAdvanced::Core::HttpHeaders;

using HttpServerAdvanced::Core::HttpContentTypes;
using HttpServerAdvanced::Core::Request;
using HttpServerAdvanced::Core::Form;
using HttpServerAdvanced::Core::RawBody;
using PostBodyData = HttpServerAdvanced::KeyValuePairView<String, String>;
using HttpServerAdvanced::Core::HandlerMatcher;
using HttpServerAdvanced::HttpUtility;
using HttpServerAdvanced::StaticFiles::StaticFiles;
using HttpServerAdvanced::StaticFiles::StaticFilesBuilder;
using HttpServerAdvanced::Core::BasicAuth;
using HttpServerAdvanced::Core::CrossOriginRequestSharing;
using Response = HttpServerAdvanced::Core::IHttpHandler::HandlerResult;
using Uri = HttpServerAdvanced::Core::HandlerMatcher;



