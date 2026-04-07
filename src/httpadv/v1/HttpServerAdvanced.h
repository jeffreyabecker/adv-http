// dont do a pragma here because its a wrapper around other include files
// #pragma once

// Core HTTP types and utilities
#include "httpadv/v1/core/Defines.h"
#include "httpadv/v1/core/HttpContentTypes.h"
#include "httpadv/v1/core/HttpRequestContext.h"
#include "httpadv/v1/core/HttpContextPhase.h"
#include "httpadv/v1/core/HttpHeader.h"
#include "httpadv/v1/core/HttpHeaderCollection.h"
#include "httpadv/v1/core/HttpMethod.h"
#include "httpadv/v1/core/HttpStatus.h"
#include "httpadv/v1/core/HttpTimeouts.h"
#include "httpadv/v1/core/IHttpContextHandlerFactory.h"
#include "httpadv/v1/transport/ByteStream.h"
#include "httpadv/v1/transport/TransportTraits.h"


// HTTP Handlers
#include "httpadv/v1/handlers/BufferedStringBodyHandler.h"
#include "httpadv/v1/handlers/BufferingHttpHandlerBase.h"
#include "httpadv/v1/handlers/FormBodyHandler.h"
#include "httpadv/v1/handlers/HandlerRestrictions.h"
#include "httpadv/v1/handlers/HandlerTypes.h"
#include "httpadv/v1/handlers/HttpHandler.h"
#include "httpadv/v1/handlers/IHandlerProvider.h"
#include "httpadv/v1/handlers/IHttpHandler.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "httpadv/v1/handlers/JsonBodyHandler.h"
#endif
#include "httpadv/v1/handlers/MultipartFormDataHandler.h"
#include "httpadv/v1/handlers/RawBodyHandler.h"

// HTTP Response types
#include "httpadv/v1/response/ChunkedHttpResponseBodyStream.h"
#include "httpadv/v1/response/FormResponse.h"
#include "httpadv/v1/response/HttpResponse.h"
#include "httpadv/v1/response/HttpResponseIterators.h"
#include "httpadv/v1/response/IHttpResponse.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "httpadv/v1/response/JsonResponse.h"
#endif
#include "httpadv/v1/response/StringResponse.h"

// Request routing
#include "httpadv/v1/routing/BasicAuthentication.h"
#include "httpadv/v1/routing/CrossOriginRequestSharing.h"
#include "httpadv/v1/routing/HandlerBuilder.h"
#include "httpadv/v1/routing/HandlerMatcher.h"
#include "httpadv/v1/routing/HandlerProviderRegistry.h"
#include "httpadv/v1/routing/ProviderRegistryBuilder.h"
#include "httpadv/v1/routing/ReplaceVariables.h"


// HTTP pipeline

#include "httpadv/v1/pipeline/HttpPipeline.h"
#include "httpadv/v1/pipeline/IPipelineHandler.h"
#include "httpadv/v1/pipeline/NetClient.h"
#include "httpadv/v1/pipeline/PipelineError.h"
#include "httpadv/v1/pipeline/PipelineHandleClientResult.h"
#include "httpadv/v1/pipeline/RequestParser.h"
#include "lumalink/platform/transport/TransportTraits.h"
#include "lumalink/platform/TransportFactory.h"
#include "llhttp/include/llhttp.h"


// Static file serving
#include "httpadv/v1/staticfiles/AggregateFileLocator.h"
#include "httpadv/v1/staticfiles/DefaultFileLocator.h"
#include "httpadv/v1/staticfiles/FileLocator.h"
#include "httpadv/v1/staticfiles/StaticFileHandler.h"
#include "httpadv/v1/staticfiles/StaticFilesBuilder.h"

// Stream utilities
#include "httpadv/v1/streams/Base64Stream.h"
//
#include "httpadv/v1/streams/Iterators.h"
#include "httpadv/v1/streams/Streams.h"
#include "httpadv/v1/streams/UriStream.h"

// Utility functions
#include "httpadv/v1/util/HttpUtility.h"
#include "httpadv/v1/util/KeyValuePairView.h"
#include "httpadv/v1/util/UriView.h"

// Server implementations
#include "httpadv/v1/server/HttpServerBase.h"
#include "httpadv/v1/server/WebServer.h"
#include "httpadv/v1/server/WebServerBuilder.h"
#include "httpadv/v1/server/WebServerConfig.h"

// Request handler factory
#include "httpadv/v1/core/HttpContextHandlerFactory.h"
