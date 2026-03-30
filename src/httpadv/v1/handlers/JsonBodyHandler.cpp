#include "JsonBodyHandler.h"
#include "../routing/HandlerMatcher.h"

#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1

namespace httpadv::v1::handlers {
IHttpHandler::HandlerResult
JsonBodyHandler::handleBody(httpadv::v1::core::HttpContext &context, std::vector<uint8_t> &&body) {
  context.items().erase(Json::DeserializationErrorItemKey);

  auto params = extractor_(context);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    context.items()[Json::DeserializationErrorItemKey] = error;
  }

  return handler_(context, std::move(params), std::move(doc));
}

Json::Invocation Json::curryWithoutParams(InvocationWithoutParams handler) {
  return [handler](httpadv::v1::core::HttpContext &context, RouteParameters &&,
                   JsonDocument &&jsonData) {
    return handler(context, std::move(jsonData));
  };
}

IHttpHandler::Factory Json::makeFactory(Invocation handler,
                                        ExtractArgsFromRequest extractor) {
  return [handler,
          extractor](httpadv::v1::core::HttpContext &context) -> std::unique_ptr<IHttpHandler> {
    auto params = extractor(context);
    return std::make_unique<JsonBodyHandler>(
        handler,
        ExtractArgsFromRequest([params](httpadv::v1::core::HttpContext &c) { return params; }));
  };
}

Json::Invocation
Json::curryInterceptor(IHttpHandler::InterceptorCallback interceptor,
                       Invocation handler) {
  return [interceptor, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params,
                                JsonDocument &&jsonData) {
    return interceptor(context, [handler, params = std::move(params),
                                 jsonData = std::move(jsonData)](
                                    httpadv::v1::core::HttpContext &context) mutable {
      return handler(context, std::move(params), std::move(jsonData));
    });
  };
}

Json::Invocation
Json::applyFilter(IHttpHandler::InterceptorCallback interceptor,
                  Invocation handler) {
  return [interceptor, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params,
                                JsonDocument &&jsonData) {
    return interceptor(context, [handler, params = std::move(params),
                                 jsonData = std::move(jsonData)](
                                    httpadv::v1::core::HttpContext &context) mutable {
      return handler(context, std::move(params), std::move(jsonData));
    });
  };
}

Json::Invocation Json::applyResponseFilter(IHttpResponse::ResponseFilter filter,
                                           Invocation handler) {
  return [filter, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params,
                           JsonDocument &&jsonData) {
    auto response = handler(context, std::move(params), std::move(jsonData));
    if (!response.isResponse()) {
      return response;
    }

    response.response = filter(std::move(response.response));
    return response;
  };
}
} // namespace HttpServerAdvanced

#endif
