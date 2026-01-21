#pragma once

#include "./HttpPipeline.h"
#include "./IPipelineHandler.h"
#include "./PipelineHandleClientResult.h"
#include "./HttpTimeouts.h"

#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>
#include <any>

namespace HttpServerAdvanced
{

    class HttpServerBase
    {
    public:
        HttpServerBase();
        virtual ~HttpServerBase();

        void handleClient();

        virtual void begin();
        virtual void end();

        HttpTimeouts &timeouts();
        void setTimeouts(const HttpTimeouts &timeouts);

        std::map<String, std::any> &items() const;

        bool hasService(const String &serviceName) const;
        HttpServerBase &use(std::function<void(HttpServerBase &)> setupFunc);

        template <typename T>
        T *getService(const String &serviceName) const
        {
            auto it = items_.find(serviceName);
            if (it != items_.end())
            {
                if (auto ptr = std::any_cast<T>(&it->second))
                {
                    return ptr;
                }
            }
            return nullptr;
        }
        void addService(HttpServerBase &server, const String &serviceName, std::any serviceInstance);
        template <typename TServiceInstance>
        void addServiceOnce(HttpServerBase &server, const String &serviceName, TServiceInstance serviceInstance)
        {
            if (!server.hasService(serviceName))
            {
                addService(server, serviceName, std::forward<TServiceInstance>(serviceInstance));
            }
        }
        template <typename TServiceInstance>
        void addService(HttpServerBase &server, const String &serviceName, TServiceInstance serviceInstance)
        {
            items_[serviceName] = std::make_any<TServiceInstance>(serviceInstance);
        }
        void setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory);

    protected:
        std::function<PipelineHandlerPtr(HttpServerBase &)> pipelineHandlerFactory_;
        std::unique_ptr<HttpPipeline> currentPipeline_;
        HttpTimeouts timeouts_;
        mutable std::map<String, std::any> items_;

        virtual std::unique_ptr<IClient> accept() = 0;
    };

} // namespace HttpServerAdvanced