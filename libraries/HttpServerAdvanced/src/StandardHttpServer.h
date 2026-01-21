#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "./HttpServerBase.h"

namespace HttpServerAdvanced
{
    template <typename TServer = WiFiServer, typename TClient = WiFiClient, uint16_t DefaultPort = 80>
    class HttpServer : public HttpServerAdvanced::HttpServerBase
    {
    protected:
        class ClientWrapper : public IClient
        {
        public:
            /**
             * @brief Constructor for ClientWrapper.
             *
             * @param client A unique pointer to the client instance to wrap.
             */
            explicit ClientWrapper(std::unique_ptr<TClient> client) : connection_(std::move(client)) {}
            ~ClientWrapper() override = default;

            /**
             * @brief Writes data to the client.
             *
             * @param buf Pointer to the buffer containing data to write.
             * @param size Number of bytes to write.
             * @return The number of bytes actually written.
             */
            std::size_t write(const uint8_t *buf, std::size_t size) override
            {
                return connection_->write(buf, size);
            }

            /**
             * @brief Returns the number of bytes available for reading.
             *
             * @return The number of bytes available in the receive buffer.
             */
            int available() override
            {
                return connection_->available();
            }

            /**
             * @brief Reads data from the client.
             *
             * @param rbuf Pointer to the buffer where read data will be stored.
             * @param size Maximum number of bytes to read.
             * @return The number of bytes actually read, or -1 on error.
             */
            int read(uint8_t *rbuf, std::size_t size) override
            {
                return connection_->read(rbuf, size);
            }

            /**
             * @brief Flushes any buffered data.
             *
             * Ensures that all pending data is sent.
             */
            void flush() override
            {
                connection_->flush();
            }

            /**
             * @brief Stops the connection.
             *
             * Closes the connection and releases resources.
             */
            void stop() override
            {
                connection_->stop();
            }

            /**
             * @brief Returns the current connection status.
             *
             * @return The connection status as a ConnectionStatus enum value.
             */
            ConnectionStatus status() override
            {
                return static_cast<ConnectionStatus>(connection_->status());
            }

            /**
             * @brief Checks if the client is connected.
             *
             * @return Non-zero if connected, zero otherwise.
             */
            uint8_t connected() override
            {
                return connection_->connected();
            }

            /**
             * @brief Gets the remote IP address.
             *
             * @return The IP address of the remote endpoint.
             */
            IPAddress remoteIP() override
            {
                return connection_->remoteIP();
            }

            /**
             * @brief Gets the remote port number.
             *
             * @return The port number of the remote endpoint.
             */
            uint16_t remotePort() override
            {
                return connection_->remotePort();
            }

            /**
             * @brief Gets the local IP address.
             *
             * @return The IP address of the local endpoint.
             */
            IPAddress localIP() override
            {
                return connection_->localIP();
            }

            /**
             * @brief Gets the local port number.
             *
             * @return The port number of the local endpoint.
             */
            uint16_t localPort() override
            {
                return connection_->localPort();
            }

            /**
             * @brief Configures the underlying connection.
             *
             * Allows customization of the client connection via a callback.
             *
             * @param callback A function to configure the client.
             */
            void configureConnection(std::function<void(TClient *)> callback)
            {
                if (callback)
                {
                    callback(connection_.get());
                }
            }
            virtual void setTimeout(uint32_t timeoutMs) override
            {
                connection_->setTimeout(timeoutMs);
            }
            virtual uint32_t getTimeout() const override
            {
                return connection_->getTimeout();
            }

        private:
            std::unique_ptr<TClient> connection_;
        };

    public:
        HttpServer(uint16_t port = DefaultPort, const IPAddress &ip = IPAddress(IPADDR_ANY))
            : server_(ip, port)
        {
        }

        void begin() override
        {
            HttpServerAdvanced::HttpServerBase::begin();
            server_.begin();
        }
        void end() override
        {
            server_.end();
        }
        void configureConnection(std::function<void(TServer *)> callback)
        {
            server_.configureConnection(callback);
        }

    protected:
        std::unique_ptr<IClient> accept() override
        {
            TClient client = server_.accept();
            if (client)
            {
                return std::make_unique<ClientWrapper>(std::make_unique<TClient>(std::move(client)));
            }
            return nullptr;
        }
        TServer server_;
    };

} // namespace HttpServerAdvanced
