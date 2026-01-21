#pragma once
#include "../util/Util.h"
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

namespace HttpServerAdvanced::Pipeline
{

    class HttpServerBase
    {
    public:
        HttpServerBase(std::function<PipelineHandlerPtr(HttpServerBase &)> pipelineHandlerFactory)
            : pipelineHandlerFactory_(pipelineHandlerFactory),
              currentPipeline_(nullptr)
        {
        }
        virtual ~HttpServerBase()
        {
            end();
        }

        void handleClient()
        {
            if (pipelineHandlerFactory_)
            {
                if (!currentPipeline_)
                {
                    std::unique_ptr<Stream> accepted = accept();
                    if (accepted)
                    {
                        PipelineHandlerPtr handler = pipelineHandlerFactory_(*this);
                        currentPipeline_ = std::make_unique<HttpPipeline>(
                            std::make_unique<IClient>(std::move(accepted)),
                            *this,
                            timeouts_,
                            std::move(handler));

                       
                    }
                }
                if (currentPipeline_)
                {
                    auto result = currentPipeline_->handleClient();
                    if (isPipelineHandleClientResultFinal(result))
                    {
                        currentPipeline_ = nullptr;
                    }
                }
            }
            else
            {
                assert(false && "No Pipeline Hander Factory was setup");
            }
        }

        virtual void begin() {}
        virtual void end()
        {
            currentPipeline_ = nullptr;
        }

        HttpTimeouts &timeouts()
        {
            return timeouts_;
        }
        void setTimeouts(const HttpTimeouts &timeouts)
        {
            timeouts_ = timeouts;
        }

        std::map<String, std::any> &items() const
        {
            return items_;
        }

        bool hasService(const String &serviceName) const
        {
            return items_.find(serviceName) != items_.end();
        }
        HttpServerBase &use(std::function<void(HttpServerBase &)> setupFunc)
        {
            setupFunc(static_cast<HttpServerBase &>(*this));
            return static_cast<HttpServerBase &>(*this);
        }

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
        void addService(HttpServerBase &server, const String &serviceName, std::any serviceInstance)
        {
            items_[serviceName] = serviceInstance;
        }
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
        void setPipelineHandlerFactory(std::function<PipelineHandlerPtr(HttpServerBase &)> factory)
        {
            pipelineHandlerFactory_ = factory;
        }

    protected:
        std::function<PipelineHandlerPtr(HttpServerBase &)> pipelineHandlerFactory_;
        std::unique_ptr<HttpPipeline> currentPipeline_;
        HttpTimeouts timeouts_;
        mutable std::map<String, std::any> items_;

        virtual std::unique_ptr<Stream> accept() = 0;
    };

} // namespace HttpServerAdvanced