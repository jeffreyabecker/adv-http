// dont do a pragma here because its a wrapper around other include files
// #pragma once

// Core HTTP types and utilities
#include "compat/Compat.h"
#include "core/Buffer.h"
#include "core/Defines.h"
#include "core/HttpContentTypes.h"
#include "core/HttpHeader.h"
#include "core/HttpHeaderCollection.h"
#include "core/HttpMethod.h"
#include "core/HttpRequest.h"
#include "core/HttpRequestPhase.h"
#include "core/HttpStatus.h"
#include "core/HttpTimeouts.h"
#include "core/IHttpRequestHandlerFactory.h"

// HTTP Handlers
#include "handlers/BufferedStringBodyHandler.h"
#include "handlers/BufferingHttpHandlerBase.h"
#include "handlers/FormBodyHandler.h"
#include "handlers/HandlerRestrictions.h"
#include "handlers/HandlerTypes.h"
#include "handlers/HttpHandler.h"
#include "handlers/IHandlerProvider.h"
#include "handlers/IHttpHandler.h"
#include "handlers/JsonBodyHandler.h"
#include "handlers/MultipartFormDataHandler.h"
#include "handlers/RawBodyHandler.h"

// HTTP Response types
#include "response/ChunkedHttpResponseBodyStream.h"
#include "response/FormResponse.h"
#include "response/HttpResponse.h"
#include "response/HttpResponseIterators.h"
#include "response/IHttpResponse.h"
#include "response/JsonResponse.h"
#include "response/StringResponse.h"

// Request routing
#include "routing/HandlerBuilder.h"
#include "routing/HandlerMatcher.h"
#include "routing/HandlerProviderRegistry.h"
#include "routing/ProviderRegistryBuilder.h"
#include "routing/BasicAuthentication.h"
#include "routing/CrossOriginRequestSharing.h"

// HTTP pipeline
#include "pipeline/HttpPipeline.h"
#include "pipeline/IPipelineHandler.h"
#include "pipeline/NetClient.h"
#include "pipeline/PipelineError.h"
#include "pipeline/PipelineHandleClientResult.h"
#include "pipeline/RequestParser.h"

// Static file serving
#include "staticfiles/AggregateFileLocator.h"
#ifdef ARDUINO
#include "staticfiles/ArduinoStaticFiles.h"
#endif
#include "staticfiles/DefaultFileLocator.h"
#include "staticfiles/FileLocator.h"
#include "staticfiles/StaticFileHandler.h"
#include "staticfiles/StaticFilesBuilder.h"

// Stream utilities
#include "streams/ByteStream.h"
#include "streams/Base64Stream.h"
#include "streams/Iterators.h"
#include "streams/Streams.h"
#include "streams/UriStream.h"

// Utility functions
#include "util/HttpUtility.h"
#include "util/KeyValuePairView.h"
#include "util/StringUtility.h"
#include "util/StringView.h"
#include "util/UriView.h"

// Server implementations
#include "server/HttpServerBase.h"
#include "server/WebServer.h"
#include "server/WebServerBuilder.h"
#include "server/WebServerConfig.h"

// Request handler factory
#include "core/HttpRequestHandlerFactory.h"

using HttpServerAdvanced::Form;
using HttpServerAdvanced::GetRequest;
using HttpServerAdvanced::RawBody;
using HttpServerAdvanced::WebServerBuilder;
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
using HttpServerAdvanced::Json;
using HttpServerAdvanced::JsonResponse;
#endif
using HttpServerAdvanced::Buffered;
using HttpServerAdvanced::Multipart;
using Upload = HttpServerAdvanced::Multipart;

using HttpServerAdvanced::FormResponse;
using HttpServerAdvanced::HttpContentTypes;
using HttpServerAdvanced::HttpHandler;
using HttpServerAdvanced::HttpHeader;
using HttpServerAdvanced::HttpMethod;
using HttpServerAdvanced::HttpRequest;
using HttpServerAdvanced::HttpResponse;
using HttpServerAdvanced::HttpStatus;
using HttpServerAdvanced::IHttpResponse;
using HttpServerAdvanced::StringResponse;

using QueryParameter = HttpServerAdvanced::WebUtility::QueryParameter;
using QueryParameters = HttpServerAdvanced::WebUtility::QueryParameters;
using PostBodyData = QueryParameters;
using HttpServerAdvanced::BasicAuth;
using HttpServerAdvanced::CrossOriginRequestSharing;
using HttpServerAdvanced::HandlerMatcher;
using HttpServerAdvanced::StaticFiles;
using HttpServerAdvanced::WebUtility;
;
using HttpServerAdvanced::StaticFilesBuilder;
using Response = HttpServerAdvanced::IHttpHandler::HandlerResult;
using Uri = HttpServerAdvanced::HandlerMatcher;
using HttpServerAdvanced::MultipartFormDataBuffer;
using HttpServerAdvanced::MultipartStatus;
using HttpServerAdvanced::RawBodyBuffer;
using HttpServerAdvanced::WebServerConfig;
using WebServer = HttpServerAdvanced::FriendlyWebServer<HttpServerAdvanced::HttpServerBase>;
