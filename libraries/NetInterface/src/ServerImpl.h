#pragma once

#include "IServer.h"
#include "ClientImpl.h"
#include <type_traits>
#include <memory>
#include <utility>
#include <functional>

namespace NetInterface
{
    /**
     * @brief Implementation of ServerWrapper for a specific Server type T.
     *
     * T is required to implement the server interface through SFINAE.
     *
     * @tparam TServer The server type to wrap.
     */
    template <typename TServer>
    class ServerImpl : public IServer
    {

    public:
        /**
         * @brief Constructor for ServerImpl.
         *
         * Uses a variadic template to allow for different server types.
         *
         * @tparam Args The argument types for the server constructor.
         * @param args The arguments to forward to the server constructor.
         */
        template <typename... Args>
        ServerImpl(Args &&...args) : connection_(std::forward<Args>(args)...)
        {
            AsyncProcessing.addHandler(this);
        }

        /**
         * @brief Destructor for ServerImpl.
         *
         * Ensures proper cleanup and marks override.
         */
        ~ServerImpl() override
        {
            AsyncProcessing.removeHandler(this);
        }

        /**
         * @brief Accepts a new client connection.
         *
         * @return A unique pointer to the accepted IClient instance, or nullptr if no client is available.
         */
        std::unique_ptr<IClient> accept() override
        {
            // our wrapper implements a different behavior from the underlying server accept()
            // we return a nullptr if no client is available, rather than a null/invalid client object
            auto client = connection_.accept();
            if (client)
            {
                using ClientType = typename std::remove_reference<decltype(client)>::type;
                auto result = std::unique_ptr<IClient>(new ClientImpl<ClientType>(client));
                if(timeoutMs_ > 0)
                {
                    result->setTimeout(timeoutMs_);
                }
                return result;
            }
            return nullptr;
        }

        /**
         * @brief Configures the underlying connection.
         *
         * Allows customization of the server connection via a callback.
         *
         * @param callback A function to configure the server.
         */
        void configureConnection(std::function<void(T *)> callback)
        {
            if (callback)
            {
                callback(&connection_);
            }
        }

        /**
         * @brief Starts listening on the server connection.
         */
        void begin() override
        {
            connection_.begin();
        }

        void begin(uint16_t port) override
        {
            connection_.begin(port);
        }

        /**
         * @brief Gets the current connection status of the server.
         *
         * @return The connection status as a ConnectionStatus enum value.
         */
        ConnectionStatus status() override
        {
            return static_cast<ConnectionStatus>(connection_.status());
        }

        /**
         * @brief Gets the port number the server is listening on.
         *
         * @return The port number.
         */
        uint16_t port() const override
        {
            return connection_.port();
        }

        /**
         * @brief Stops the server.
         */
        void end() override
        {
            connection_.end();
        }
        /**
         * @brief Sets the timeout for client operations.
         *
         * @param timeoutMs The timeout duration in milliseconds.
         */
        void setDefaultTimeout(uint32_t timeoutMs) override
        {
            timeoutMs_ = timeoutMs;
        }
        /**
         * @brief Gets the current timeout for client operations.
         *
         * @return The timeout duration in milliseconds.
         */
        uint32_t getDefaultTimeout() const override
        {
            if (!clients_.empty())
            {
                return clients_.front().client->getTimeout();
            }
            return 0; // or some default value
        }

    private:
        TServer connection_;
        uint32_t timeoutMs_ = 0;
    };

    template <typename TServer, typename = std::enable_if_t<std::is_convertible_v<TServer, IServer>>>
    class AsyncServerImpl : public TServer, public IAsyncServer
    {
    private:
        struct ClientRef
        {
            std::shared_ptr<IClient> client;
            IAsyncServer::CallbackHandler onPoll;
            IAsyncServer::CallbackHandler onDisconnected;
        };

    public:
        template <typename... Args>
        AsyncServerImpl(Args &&...args)
            : TServer(std::forward<Args>(args)...)
        {
            AsyncProcessing.addHandler(this);
        }

        /**
         * @brief Destructor for AsyncServer.
         */
        ~AsyncServer() override
        {
            AsyncProcessing.removeHandler(this);
        }

        void onAccept(CallbackHandler acceptedHandler, CallbackHandler pollHandler = nullptr, CallbackHandler disconnectedHandler = nullptr) override
        {
            acceptHandler_ = acceptedHandler;
            pollHandler_ = pollHandler;
            disconnectedHandler_ = disconnectedHandler;
        }

        void setPollCallback(std::shared_ptr<IClient> client, CallbackHandler pollHandler) override
        {
            for (auto &clientRef : clients_)
            {
                if (clientRef.client == client)
                {
                    clientRef.onPoll = pollHandler;
                    break;
                }
            }
        }
        void setDisconnectedCallback(std::shared_ptr<IClient> client, CallbackHandler disconnectedHandler) override
        {
            for (auto &clientRef : clients_)
            {
                if (clientRef.client == client)
                {
                    clientRef.onDisconnected = disconnectedHandler;
                    break;
                }
            }
        }

        void handleIncomingRequests() override
        {
            if (acceptHandler_)
            {
                auto client = TServer::accept();
                if (client)
                {
                    ClientRef clientRef{std::shared_ptr<IClient>(std::move(client)), pollHandler_, disconnectedHandler_};

                    clients_.push_back(clientRef);
                    acceptHandler_(*this, clientRef.client);
                }
            }
            for (auto it = clients_.begin(); it != clients_.end();)
            {
                if (!(*it).client->connected())
                {
                    if ((*it).onDisconnected)
                    {
                        (*it).onDisconnected(*this, (*it).client);
                    }
                    it = clients_.erase(it);
                }
                else
                {
                    if ((*it).onPoll)
                    {
                        (*it).onPoll(*this, (*it).client);
                    }
                    ++it;
                }
            }
        }

    private:
        IAsyncServer::CallbackHandler acceptHandler_ = nullptr;
        IAsyncServer::CallbackHandler pollHandler_ = nullptr;
        IAsyncServer::CallbackHandler disconnectedHandler_ = nullptr;
        std::list<ClientRef> clients_; // Store active clients to manage their lifetimes
    };
} // namespace NetInterface
