// dont do a pragma here because its a wrapper around other include files
// #pragma once

// TODO: This is the root include of the library. It should include all public headers.
//  Add includes for all public headers here.
//  Add using statements to pull members out of namespaces .
#include "./StandardHttpServer.h"
#include "./SecureHttpServer.h"
#include "./CoreServices.h"
#include "./KeyValuePairView.h"
#include "./HttpUtility.h"
#include "./UriView.h"
#include "./BasicAuthentication.h"
#include "./CrossOriginRequestSharing.h"
#include "./StaticFilesBuilder.h"

using HttpServerAdvanced::CoreServices;
using HttpServerAdvanced::HttpServer;
using HttpServerAdvanced::SecureHttpServer;
using HttpServerAdvanced::SecureHttpServerConfig;
using WebServerBuilder = HttpServerAdvanced::CoreServicesBuilder;
using HttpServerAdvanced::Form;
using HttpServerAdvanced::RawBody;
using HttpServerAdvanced::Request;
using HttpServerAdvanced::Json;
using HttpServerAdvanced::Multipart;
using Upload = HttpServerAdvanced::Multipart;

using HttpServerAdvanced::HttpContentTypes;
using HttpServerAdvanced::HttpRequest;
using HttpServerAdvanced::HttpHandler;
using HttpServerAdvanced::HttpHeader;
using HttpServerAdvanced::HttpMethod;
using HttpServerAdvanced::HttpResponse;
using HttpServerAdvanced::HttpStatus;
using HttpServerAdvanced::IHttpResponse;

using PostBodyData = HttpServerAdvanced::KeyValuePairView<String, String>;
using HttpServerAdvanced::BasicAuth;
using HttpServerAdvanced::CrossOriginRequestSharing;
using HttpServerAdvanced::HandlerMatcher;
using HttpServerAdvanced::HttpUtility;
using HttpServerAdvanced::StaticFiles;;
using HttpServerAdvanced::StaticFilesBuilder;
using Response = HttpServerAdvanced::IHttpHandler::HandlerResult;
using Uri = HttpServerAdvanced::HandlerMatcher;
