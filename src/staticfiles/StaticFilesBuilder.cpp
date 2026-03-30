#include "StaticFilesBuilder.h"

namespace httpadv::v1::staticfiles {
StaticFilesBuilder::StaticFilesBuilder(
    IFileSystem &fs, std::function<void(StaticFilesBuilder &)> setupFunc)
    : setupFunc_(setupFunc), fileLocator_(fs) {}

StaticFilesBuilder::~StaticFilesBuilder() {}

void StaticFilesBuilder::init(
    httpadv::v1::server::WebServerBuilder &coreBuilder) {
  if (setupFunc_) {
    setupFunc_(*this);
  }
  auto &handlerFactory = coreBuilder.handlerProviders();
  handlerFactory.addAt<StaticFileHandlerFactory>(
      0, std::make_shared<DefaultFileLocator>(fileLocator_),
      coreBuilder.contentTypes(), std::move(responseFilterRules_),
      std::move(interceptorRules_), std::move(requestPredicateRules_));
}

StaticFilesBuilder &StaticFilesBuilder::setPathPredicate(
    std::function<bool(std::string_view)> predicate) {
  fileLocator_.setPathPredicate(predicate);
  return *this;
}

StaticFilesBuilder &StaticFilesBuilder::setPathMapper(
    std::function<std::string(std::string_view)> mapper) {
  fileLocator_.setPathMapper(mapper);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setRequestPathPrefixes(std::string_view prefix,
                                           std::string_view excludePrefix) {
  fileLocator_.setRequestPathPrefixes(prefix, excludePrefix);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setFilesystemContentRoot(std::string_view root) {
  fileLocator_.setFilesystemContentRoot(root);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::apply(HandlerMatcher matcher,
                          IHttpResponse::ResponseFilter filter) {
  if (!filter) {
    return *this;
  }

  responseFilterRules_.push_back(
      {std::move(matcher), std::move(filter)});
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::apply(std::string_view uriPattern,
                          IHttpResponse::ResponseFilter filter) {
  return apply(HandlerMatcher(uriPattern), std::move(filter));
}

StaticFilesBuilder &
StaticFilesBuilder::apply(const char *uriPattern,
                          IHttpResponse::ResponseFilter filter) {
  return apply(HandlerMatcher(uriPattern), std::move(filter));
}

StaticFilesBuilder &
StaticFilesBuilder::with(HandlerMatcher matcher,
                         IHttpHandler::InterceptorCallback wrapper) {
  if (!wrapper) {
    return *this;
  }

  interceptorRules_.push_back({std::move(matcher), std::move(wrapper)});
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::with(std::string_view uriPattern,
                         IHttpHandler::InterceptorCallback wrapper) {
  return with(HandlerMatcher(uriPattern), std::move(wrapper));
}

StaticFilesBuilder &
StaticFilesBuilder::with(const char *uriPattern,
                         IHttpHandler::InterceptorCallback wrapper) {
  return with(HandlerMatcher(uriPattern), std::move(wrapper));
}

StaticFilesBuilder &
StaticFilesBuilder::filterRequest(HandlerMatcher matcher,
                                  IHttpHandler::Predicate predicate) {
  if (!predicate) {
    return *this;
  }

  requestPredicateRules_.push_back(
      {std::move(matcher), std::move(predicate)});
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::filterRequest(std::string_view uriPattern,
                                  IHttpHandler::Predicate predicate) {
  return filterRequest(HandlerMatcher(uriPattern), std::move(predicate));
}

StaticFilesBuilder &
StaticFilesBuilder::filterRequest(const char *uriPattern,
                                  IHttpHandler::Predicate predicate) {
  return filterRequest(HandlerMatcher(uriPattern), std::move(predicate));
}

std::function<void(WebServerBuilder &)> &
StaticFiles(IFileSystem &fs,
            std::function<void(StaticFilesBuilder &)> setupFunc) {
  auto *instance = new std::function<void(WebServerBuilder &)>(
      [setupFunc, &fs](WebServerBuilder &coreBuilder) {
        StaticFilesBuilder staticFilesBuilder(fs, setupFunc);
        staticFilesBuilder.init(coreBuilder);
      });
  return *instance;
}

} // namespace httpadv::v1::staticfiles
