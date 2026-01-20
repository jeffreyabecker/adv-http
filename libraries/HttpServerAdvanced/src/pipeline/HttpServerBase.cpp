#include "./HttpServerBase.h"
#include "./HttpPipelineTimeouts.h"
namespace HttpServerAdvanced::Pipeline
{

    HttpServerBase::~HttpServerBase()
    {
        end();
    }

    void HttpServerBase::handleClient()
    {
        if (pipelineHandlerFactory_)
        {
            if (!currentPipeline_)
            {
                std::unique_ptr<Stream> accepted = accept();
                if (accepted)
                {
                    currentPipeline_ = std::make_unique<HttpPipeline>(std::move(accepted), *this);
                    auto handler = pipelineHandlerFactory_->createHandler(*currentPipeline_);
                    currentPipeline_->setHandler(std::move(handler));   
                }
            }
            if (currentPipeline_)
            {
                auto result = currentPipeline_->handleClient();
                if (isHttpPipelineHandleClientResultFinal(result))
                {
                    currentPipeline_ = nullptr;
                }
            }
        }
        else{
            assert(false && "No Pipeline Hander Factory was setup");
        }
    }

    HttpPipelineTimeouts &HttpServerBase::timeouts()
    {
        return timeouts_;
    }

    void HttpServerBase::setTimeouts(const HttpPipelineTimeouts &timeouts)
    {
        timeouts_ = timeouts;
    }

    void HttpServerBase::setPipelineHandlerFactory(IHttpPipelineHandlerFactory *factory)
    {
        pipelineHandlerFactory_ = factory;
    }

}