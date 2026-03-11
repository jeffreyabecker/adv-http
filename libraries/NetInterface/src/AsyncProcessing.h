#pragma once
#include <list>

namespace NetInterface
{
    /**
     * @brief Interface for handling asynchronous processing in a server.
     *
     * This interface defines a callback mechanism for handling new client connections
     * in an asynchronous server implementation. It allows users to set a callback function
     * that will be called whenever a new client connects to the server.
     */
    class IHandleAsyncProcessing
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IHandleAsyncProcessing() = default;

        virtual void handleIncomingRequests() = 0;
    };

    class AsyncProcessingManager
    {
    private:
        std::list<IHandleAsyncProcessing *> handlers_;

    public:
        void handleIncomingRequests()
        {
            for (auto handler : handlers_)
            {
                handler->handleIncomingRequests();
            }
        }
        void addHandler(IHandleAsyncProcessing *handler)
        {
            handlers_.push_back(handler);
        }
        void removeHandler(IHandleAsyncProcessing *handler)
        {
            handlers_.remove(handler);
        }
    };

    AsyncProcessingManager AsyncProcessing;
} // namespace NetInterface