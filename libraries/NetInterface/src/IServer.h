#pragma once
#include "IClient.h"
#include "AsyncProcessing.h"
#include <memory>
#include <list>
#include <functional>

namespace NetInterface
{
    /**
     * @brief Interface for a generic Server type.
     */
    class IServer
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IServer() = default;

        /**
         * @brief Accepts a new client connection.
         *
         * @return A unique pointer to the accepted IClient instance, or nullptr if no client is available.
         */
        virtual std::unique_ptr<IClient> accept() = 0;

        virtual void begin() = 0;
        /**
         * @brief Initializes the server to listen on the specified port.
         * If the port is 0, the server will choose an available port automatically.
         */
        virtual void begin(uint16_t port) = 0;

        /**
         * @brief Gets the current connection status of the server.
         *
         * @return The connection status as a ConnectionStatus enum value.
         */
        virtual ConnectionStatus status() = 0;

        /**
         * @brief Gets the port number the server is listening on.
         *
         * @return The port number.
         */
        virtual uint16_t port() const = 0;

        /**
         * @brief Stops the server.
         */
        virtual void end() = 0;
        /**
         * @brief Sets the timeout for client operations.
         *
         * @param timeoutMs The timeout duration in milliseconds.
         */
        virtual void setDefaultTimeout(uint32_t timeoutMs) = 0;
        /**
         * @brief Gets the current timeout for client operations.
         *
         * @return The timeout duration in milliseconds.
         */
        virtual uint32_t getDefaultTimeout() const = 0;
    };

    class IAsyncServer : public IServer, public IHandleAsyncProcessing
    {
    public:
        using CallbackHandler = std::function<void(IAsyncServer &, std::shared_ptr<IClient>)>;

        /**
         * @brief Virtual destructor.
         */
        ~IAsyncServer() override = default;
        virtual void onAccept(CallbackHandler acceptedHandler, CallbackHandler pollHandler = nullptr, CallbackHandler disconnectedHandler = nullptr) = 0;
        virtual void setPollCallback(std::shared_ptr<IClient> client, CallbackHandler pollHandler) = 0;
        virtual void setDisconnectedCallback(std::shared_ptr<IClient> client, CallbackHandler disconnectedHandler) = 0;
    };

} // namespace NetInterface
