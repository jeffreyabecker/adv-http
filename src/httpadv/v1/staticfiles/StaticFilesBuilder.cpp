#include "StaticFilesBuilder.h"

namespace httpadv::v1::staticfiles {
StaticFilesBuilder::StaticFilesBuilder(
    IFileSystem &fs, std::function<void(StaticFilesBuilder &)> setupFunc)
  : setupFunc_(setupFunc), filesystem_(&fs) {}

StaticFilesBuilder::~StaticFilesBuilder() {}

void StaticFilesBuilder::init(
    httpadv::v1::server::WebServerBuilder &coreBuilder) {
  if (setupFunc_) {
    setupFunc_(*this);
  }

  if (!fileLocator_) {
    auto defaultFileLocator = std::make_unique<DefaultFileLocator>(filesystem_);
    for (auto &action : defaultFileLocatorActions_) {
      action(*defaultFileLocator);
    }
    fileLocator_ = std::move(defaultFileLocator);
  }

  auto &handlerFactory = coreBuilder.handlerProviders();
  handlerFactory.addAt<StaticFileHandlerFactory>(
      0, std::move(fileLocator_),
      coreBuilder.contentTypes(), std::move(responseFilterRules_),
      std::move(interceptorRules_), std::move(requestPredicateRules_),
      std::move(notFoundRequestPathResolver_));
}

StaticFilesBuilder &StaticFilesBuilder::setPathPredicate(
    std::function<bool(std::string_view)> predicate) {
  if (!predicate) {
    return *this;
  }

  defaultFileLocatorActions_.push_back(
      [predicate = std::move(predicate)](DefaultFileLocator &locator) mutable {
        locator.setPathPredicate(std::move(predicate));
      });
  return *this;
}

StaticFilesBuilder &StaticFilesBuilder::setPathMapper(
    std::function<std::string(std::string_view)> mapper) {
  if (!mapper) {
    return *this;
  }

  defaultFileLocatorActions_.push_back(
      [mapper = std::move(mapper)](DefaultFileLocator &locator) mutable {
        locator.setPathMapper(std::move(mapper));
      });
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setRequestPathPrefixes(std::string_view prefix,
                                           std::string_view excludePrefix) {
  defaultFileLocatorActions_.push_back(
      [prefix = std::string(prefix),
       excludePrefix = std::string(excludePrefix)](
          DefaultFileLocator &locator) {
        locator.setRequestPathPrefixes(prefix, excludePrefix);
      });
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setFilesystemContentRoot(std::string_view root) {
  defaultFileLocatorActions_.push_back(
      [root = std::string(root)](DefaultFileLocator &locator) {
        locator.setFilesystemContentRoot(root);
      });
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::setFileLocator(std::unique_ptr<FileLocator> locator) {
  fileLocator_ = std::move(locator);
  return *this;
}

StaticFilesBuilder &
StaticFilesBuilder::onNotFound(std::string_view requestPath) {
  const std::string fallbackPath(requestPath);
  notFoundRequestPathResolver_ =
      [fallbackPath](httpadv::v1::core::HttpRequestContext &)
      -> std::optional<std::string> {
    return fallbackPath;
  };
  return *this;
}

StaticFilesBuilder &StaticFilesBuilder::onNotFound(
    StaticFileHandlerFactory::MissingRequestPathResolver resolver) {
  notFoundRequestPathResolver_ = std::move(resolver);
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
