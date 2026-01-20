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
 

using HTTPServer::HttpServer;
using HTTPServer::SecureHttpServer;
using HTTPServer::SecureHttpServerConfig;
using HTTPServer::Core::CoreServices;
using WebServerBuilder = HTTPServer::Core::CoreServicesBuilder;
using HTTPServer::Core::HttpContext;
using HTTPServer::Core::HttpHandler;
using HTTPServer::Core::HttpRequest;
using HTTPServer::Core::HttpResponse;
using HTTPServer::Core::IHttpResponse;
using HTTPServer::Core::HttpStatus;
using HTTPServer::Core::HttpMethod;
using HTTPServer::Core::HttpHeader;
using HTTPServer::Core::HttpHeaders;

using HTTPServer::Core::HttpContentTypes;
using HTTPServer::Core::Request;
using HTTPServer::Core::Form;
using HTTPServer::Core::RawBody;
using PostBodyData = HTTPServer::KeyValuePairView<String, String>;
using HTTPServer::Core::HandlerMatcher;
using HTTPServer::HttpUtility;
using HTTPServer::StaticFiles::StaticFiles;
using HTTPServer::StaticFiles::StaticFilesBuilder;
using HTTPServer::Core::BasicAuth;
using HTTPServer::Core::CrossOriginRequestSharing;
using Response = HTTPServer::Core::IHttpHandler::HandlerResult;
using Uri = HTTPServer::Core::HandlerMatcher;



