// dont do a pragma here because its a wrapper around other include files
// #pragma once

// Core HTTP types and utilities
#include "lumalink/http/core/Defines.h"
#include "lumalink/http/core/HttpContentTypes.h"
#include "lumalink/http/core/HttpRequestContext.h"
#include "lumalink/http/core/HttpContextPhase.h"
#include "lumalink/http/core/HttpHeader.h"
#include "lumalink/http/core/HttpHeaderCollection.h"
#include "lumalink/http/core/HttpMethod.h"
#include "lumalink/http/core/HttpStatus.h"
#include "lumalink/http/core/HttpTimeouts.h"
#include "lumalink/http/core/IHttpContextHandlerFactory.h"
#include "lumalink/http/transport/ByteStream.h"
#include "lumalink/http/transport/TransportTraits.h"


// HTTP Handlers
#include "lumalink/http/handlers/BufferedStringBodyHandler.h"
#include "lumalink/http/handlers/BufferingHttpHandlerBase.h"
#include "lumalink/http/handlers/FormBodyHandler.h"
#include "lumalink/http/handlers/HandlerRestrictions.h"
#include "lumalink/http/handlers/HandlerTypes.h"
#include "lumalink/http/handlers/HttpHandler.h"
#include "lumalink/http/handlers/IHandlerProvider.h"
#include "lumalink/http/handlers/IHttpHandler.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "lumalink/http/handlers/JsonBodyHandler.h"
#endif
#include "lumalink/http/handlers/MultipartFormDataHandler.h"
#include "lumalink/http/handlers/RawBodyHandler.h"

// HTTP Response types
#include "lumalink/http/response/ChunkedHttpResponseBodyStream.h"
#include "lumalink/http/response/FormResponse.h"
#include "lumalink/http/response/HttpResponse.h"
#include "lumalink/http/response/HttpResponseIterators.h"
#include "lumalink/http/response/IHttpResponse.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "lumalink/http/response/JsonResponse.h"
#endif
#include "lumalink/http/response/StringResponse.h"

// Request routing
#include "lumalink/http/routing/BasicAuthentication.h"
#include "lumalink/http/routing/CrossOriginRequestSharing.h"
#include "lumalink/http/routing/HandlerBuilder.h"
#include "lumalink/http/routing/HandlerMatcher.h"
#include "lumalink/http/routing/HandlerProviderRegistry.h"
#include "lumalink/http/routing/ProviderRegistryBuilder.h"
#include "lumalink/http/routing/ReplaceVariables.h"


// HTTP pipeline

#include "lumalink/http/pipeline/HttpPipeline.h"
#include "lumalink/http/pipeline/IPipelineHandler.h"
#include "lumalink/http/pipeline/NetClient.h"
#include "lumalink/http/pipeline/PipelineError.h"
#include "lumalink/http/pipeline/PipelineHandleClientResult.h"
#include "lumalink/http/pipeline/RequestParser.h"
#include "lumalink/platform/transport/TransportTraits.h"
#include "lumalink/platform/TransportFactory.h"
#include "llhttp/include/llhttp.h"


// Static file serving
#include "lumalink/http/staticfiles/AggregateFileLocator.h"
#include "lumalink/http/staticfiles/DefaultFileLocator.h"
#include "lumalink/http/staticfiles/FileLocator.h"
#include "lumalink/http/staticfiles/StaticFileHandler.h"
#include "lumalink/http/staticfiles/StaticFilesBuilder.h"

// Stream utilities
#include "lumalink/http/streams/Base64Stream.h"
//
#include "lumalink/http/streams/Iterators.h"
#include "lumalink/http/streams/Streams.h"
#include "lumalink/http/streams/UriStream.h"

// Utility functions
#include "lumalink/http/util/HttpUtility.h"
#include "lumalink/http/util/KeyValuePairView.h"
#include "lumalink/http/util/UriView.h"

// Server implementations
#include "lumalink/http/server/HttpServerBase.h"
#include "lumalink/http/server/WebServer.h"
#include "lumalink/http/server/WebServerBuilder.h"
#include "lumalink/http/server/WebServerConfig.h"

// Request handler factory
#include "lumalink/http/core/HttpContextHandlerFactory.h"
