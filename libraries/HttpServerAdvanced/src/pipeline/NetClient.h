#pragma once

#include <Arduino.h>
#include "../core/Defines.h"
#include <memory>
#include <type_traits>
#include <utility>

namespace HttpServerAdvanced
{

    /**
     * @brief Enumeration representing the status of a network connection.
     *
     * This enum defines the various states a TCP connection can be in, based on the TCP state machine.
     */
    enum class ConnectionStatus
    {
        CLOSED = 0,      ///< Represents the non-existent state, the starting and ending point of a TCP connection where there is no connection control block (TCB).
        LISTEN = 1,      ///< The server is waiting for a connection request (SYN packet) from any remote TCP and port (passive open).
        SYN_SENT = 2,    ///< The client is waiting for a matching connection request acknowledgment (SYN+ACK) after having sent a connection request (active open).
        SYN_RCVD = 3,    ///< The server has both received and sent a connection request and is waiting for the final acknowledgment (ACK) to establish the connection.
        ESTABLISHED = 4, ///< The normal state during the data transfer phase, indicating an open connection where data can be delivered to the user.
        FIN_WAIT_1 = 5,  ///< The local application has initiated an active close and sent a FIN packet, waiting for a connection termination request from the remote TCP or an acknowledgment of the previously sent FIN.
        FIN_WAIT_2 = 6,  ///< The local side has received an acknowledgment for its FIN and is waiting for the remote TCP to send its own FIN packet to terminate the connection.
        CLOSE_WAIT = 7,  ///< The remote end has initiated a graceful close (sent a FIN), and the local stack is waiting for the local application to issue its own close request.
        CLOSING = 8,     ///< A rare state that occurs during a simultaneous close, where the local side has sent a FIN and then receives a FIN from the remote side before receiving the ACK for its own FIN.
        LAST_ACK = 9,    ///< The local side (which was in CLOSE_WAIT) has sent its own FIN and is now waiting for the final acknowledgment from the remote end.
        TIME_WAIT = 10   ///< The local end has finished its role in the connection termination and is waiting for a period (typically twice the maximum segment lifetime, 2MSL) to ensure all packets related to the old connection have expired before fully closing the TCB.
    };

    /**
     * @brief Interface for a network client.
     *
     * This abstract class defines the interface for a client that can connect to a network,
     * send and receive data, and manage connection status. All methods are pure virtual
     * and must be implemented by derived classes.
     */
    class IClient
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        virtual ~IClient() = default;

        /**
         * @brief Writes data to the client.
         *
         * @param buf Pointer to the buffer containing data to write.
         * @param size Number of bytes to write.
         * @return The number of bytes actually written.
         */
        virtual std::size_t write(const uint8_t *buf, std::size_t size) = 0;

        /**
         * @brief Returns the number of bytes available for reading.
         *
         * @return The number of bytes available in the receive buffer.
         */
        virtual int available() = 0;

        /**
         * @brief Reads data from the client.
         *
         * @param rbuf Pointer to the buffer where read data will be stored.
         * @param size Maximum number of bytes to read.
         * @return The number of bytes actually read, or -1 on error.
         */
        virtual int read(uint8_t *rbuf, std::size_t size) = 0;

        /**
         * @brief Flushes any buffered data.
         *
         * Ensures that all pending data is sent.
         */
        virtual void flush() = 0;

        /**
         * @brief Stops the connection.
         *
         * Closes the connection and releases resources.
         */
        virtual void stop() = 0;

        /**
         * @brief Returns the current connection status.
         *
         * @return The connection status as a ConnectionStatus enum value.
         */
        virtual ConnectionStatus status() = 0;

        /**
         * @brief Checks if the client is connected.
         *
         * @return Non-zero if connected, zero otherwise.
         */
        virtual uint8_t connected() = 0;

        /**
         * @brief Gets the remote IP address.
         *
         * @return The IP address of the remote endpoint.
         */
        virtual IPAddress remoteIP() = 0;

        /**
         * @brief Gets the remote port number.
         *
         * @return The port number of the remote endpoint.
         */
        virtual uint16_t remotePort() = 0;

        /**
         * @brief Gets the local IP address.
         *
         * @return The IP address of the local endpoint.
         */
        virtual IPAddress localIP() = 0;

        /**
         * @brief Gets the local port number.
         *
         * @return The port number of the local endpoint.
         */
        virtual uint16_t localPort() = 0;

        /**
         * @brief Sets the timeout for client operations.
         *
         * @param timeoutMs The timeout duration in milliseconds.
         */
        virtual void setTimeout(uint32_t timeoutMs) = 0;
        /**
         * @brief Gets the current timeout for client operations.
         *
         * @return The timeout duration in milliseconds.
         */
        virtual uint32_t getTimeout() const = 0;
    };

    /**
     * @brief Interface for a generic Server type.
     */
    class IServer
    {
    public:
        /**
         * @brief Defines the ClientType as ClientWrapper.
         */
        using ClientType = IClient;

        /**
         * @brief Virtual destructor.
         */
        virtual ~IServer() = default;

        IServer() = default;

        /**
         * @brief Constructor for IServer.
         *
         * @param ip The IP address to bind to.
         * @param port The port number to listen on.
         */
        IServer(const IPAddress &ip, uint16_t port) {}

        /**
         * @brief Accepts a new client connection.
         *
         * @return A unique pointer to the accepted IClient instance, or nullptr if no client is available.
         */
        virtual std::unique_ptr<IClient> accept() = 0;

        /**
         * @brief Starts listening on the server connection.
         */
        virtual void begin() = 0;

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
    };

    /**
     * @brief Interface for a network peer (e.g., UDP).
     *
     * This abstract class defines the interface for a peer that can send and receive
     * datagrams, join multicast groups, and manage connection status.
     */
    class IPeer
    {
    public:
        /**
         * @brief Initializes and starts listening on the specified port.
         *
         * @param port The port number to listen on.
         * @return 1 if successful, 0 if there are no sockets available to use.
         */
        virtual uint8_t begin(uint16_t port) = 0;

        /**
         * @brief Initializes and starts listening on the specified multicast IP address and port.
         *
         * @param multicast The multicast IP address to listen on.
         * @param port The port number to listen on.
         * @return 1 if successful, 0 if there are no sockets available to use.
         */
        virtual uint8_t beginMulticast(IPAddress multicast, uint16_t port) = 0;

        /**
         * @brief Finishes with the UDP connection.
         */
        virtual void end() = 0;

        /**
         * @brief Joins a multicast group and listens on the given port.
         *
         * @param interfaceAddr The local interface address.
         * @param multicast The multicast group address.
         * @param port The port number to listen on.
         * @return 1 if successful, 0 otherwise.
         */
        virtual uint8_t beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port);

        /**
         * @brief Starts building up a packet to send to the remote host.
         *
         * @param ip The IP address of the remote host.
         * @param port The port number of the remote host.
         * @return 1 if successful, 0 if there was a problem with the supplied IP address or port.
         */
        virtual int beginPacket(IPAddress ip, uint16_t port) = 0;

        /**
         * @brief Finishes the packet and sends it.
         *
         * @return 1 if the packet was sent successfully, 0 if there was an error.
         */
        virtual int endPacket() = 0;

        /**
         * @brief Writes data into the packet.
         *
         * @param buffer Pointer to the buffer containing data to write.
         * @param size Number of bytes to write.
         * @return The number of bytes written.
         */
        virtual size_t write(const uint8_t *buffer, size_t size) = 0;

        /**
         * @brief Starts processing the next available incoming packet.
         *
         * @return The size of the packet in bytes, or 0 if no packets are available.
         */
        virtual int parsePacket() = 0;

        /**
         * @brief Returns the number of bytes remaining in the current packet.
         *
         * @return The number of bytes available.
         */
        virtual int available() = 0;

        /**
         * @brief Reads data from the current packet.
         *
         * @param buffer Pointer to the buffer where read data will be stored.
         * @param len Maximum number of bytes to read.
         * @return The number of bytes actually read, or 0 if none are available.
         */
        virtual int read(uint8_t *buffer, size_t len) = 0;

        /**
         * @brief Returns the next byte from the current packet without moving on to the next byte.
         *
         * @return The next byte, or -1 if none are available.
         */
        virtual int peek() = 0;

        /**
         * @brief Finishes reading the current packet.
         */
        virtual void flush() = 0;

        /**
         * @brief Returns the IP address of the host who sent the current incoming packet.
         *
         * @return The IP address of the remote host.
         */
        virtual IPAddress remoteIP() = 0;

        /**
         * @brief Returns the port of the host who sent the current incoming packet.
         *
         * @return The port number of the remote host.
         */
        virtual uint16_t remotePort() = 0;
    };

    /**
     * @brief Implementation of ClientWrapper for a specific Client type T.
     *
     * T is required to implement the client interface through SFINAE.
     *
     * @tparam T The client type to wrap.
     */
    template <typename T>
    class ClientImpl : public IClient
    {

    public:
        /**
         * @brief Constructor for ClientImpl.
         *
         * @param client A unique pointer to the client instance to wrap.
         */
        explicit ClientImpl(T client) : connection_(client) {}

        ~ClientImpl() override
        {
            if (connection_ && connection_.connected())
            {
                connection_.stop();
            }
        }
        /**
         * @brief Writes data to the client.
         *
         * @param buf Pointer to the buffer containing data to write.
         * @param size Number of bytes to write.
         * @return The number of bytes actually written.
         */
        std::size_t write(const uint8_t *buf, std::size_t size) override
        {
            return connection_.write(buf, size);
        }

        /**
         * @brief Returns the number of bytes available for reading.
         *
         * @return The number of bytes available in the receive buffer.
         */
        int available() override
        {
            return connection_.available();
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
            return connection_.read(rbuf, size);
        }

        /**
         * @brief Flushes any buffered data.
         *
         * Ensures that all pending data is sent.
         */
        void flush() override
        {
            connection_.flush();
        }

        /**
         * @brief Stops the connection.
         *
         * Closes the connection and releases resources.
         */
        void stop() override
        {
            connection_.stop();
        }

        /**
         * @brief Returns the current connection status.
         *
         * @return The connection status as a ConnectionStatus enum value.
         */
        ConnectionStatus status() override
        {
            return static_cast<ConnectionStatus>(connection_.status());
        }

        /**
         * @brief Checks if the client is connected.
         *
         * @return Non-zero if connected, zero otherwise.
         */
        uint8_t connected() override
        {
            return connection_.connected();
        }

        /**
         * @brief Gets the remote IP address.
         *
         * @return The IP address of the remote endpoint.
         */
        IPAddress remoteIP() override
        {
            return connection_.remoteIP();
        }

        /**
         * @brief Gets the remote port number.
         *
         * @return The port number of the remote endpoint.
         */
        uint16_t remotePort() override
        {
            return connection_.remotePort();
        }

        /**
         * @brief Gets the local IP address.
         *
         * @return The IP address of the local endpoint.
         */
        IPAddress localIP() override
        {
            return connection_.localIP();
        }

        /**
         * @brief Gets the local port number.
         *
         * @return The port number of the local endpoint.
         */
        uint16_t localPort() override
        {
            return connection_.localPort();
        }

        /**
         * @brief Configures the underlying connection.
         *
         * Allows customization of the client connection via a callback.
         *
         * @param callback A function to configure the client.
         */
        void configureConnection(std::function<void(T *)> callback)
        {
            if (callback)
            {
                callback(&connection_);
            }
        }
        virtual void setTimeout(uint32_t timeoutMs) override
        {
            connection_.setTimeout(timeoutMs);
        }
        virtual uint32_t getTimeout() const override
        {
            return const_cast<T &>(connection_).getTimeout();
        }

    private:
        T connection_;
    };

    /**
     * @brief Implementation of ServerWrapper for a specific Server type T.
     *
     * T is required to implement the server interface through SFINAE.
     *
     * @tparam T The server type to wrap.
     */
    template <typename T>
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
        ServerImpl(Args &&...args) : connection_(std::forward<Args>(args)...) {}

        /**
         * @brief Destructor for ServerImpl.
         *
         * Ensures proper cleanup and marks override.
         */
        ~ServerImpl() override = default;

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
                return std::unique_ptr<IClient>(new ClientImpl<ClientType>(client));
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

    private:
        T connection_;
    };


    /**
     * @brief Implementation of PeerWrapper for a specific Peer type T.
     *
     * T is required to implement the peer interface through SFINAE.
     *
     * @tparam T The peer type to wrap.
     */
    template <typename T>
    class PeerImpl : public IPeer
    {
    public:
        /**
         * @brief Constructor for PeerImpl.
         *
         * Uses a variadic template to allow for different peer types.
         *
         * @tparam Args The argument types for the peer constructor.
         * @param args The arguments to forward to the peer constructor.
         */
        template <typename... Args>
        PeerImpl(Args &&...args) : peer_(std::forward<Args>(args)...) {}

        /**
         * @brief Destructor for PeerImpl.
         */
        ~PeerImpl() override = default;

        uint8_t begin(uint16_t port) override
        {
            return peer_.begin(port);
        }

        uint8_t beginMulticast(IPAddress multicast, uint16_t port) override
        {
            return peer_.beginMulticast(multicast, port);
        }
        // If T provides a member function `stop`, prefer calling `stop()`; otherwise call `end()`.
        template <typename U = T, typename std::enable_if<
                      !std::is_member_function_pointer<decltype(&U::stop)>::value,
                      int>::type = 0>
        void end() override
        {
            peer_.end();
        }

        template <typename U = T, typename std::enable_if<
                      std::is_member_function_pointer<decltype(&U::stop)>::value,
                      int>::type = 0>
        void end() override
        {
            peer_.stop();
        }

        uint8_t beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port) override
        {
            return peer_.beginMulticast(interfaceAddr, multicast, port);
        }

        int beginPacket(IPAddress ip, uint16_t port) override
        {
            return peer_.beginPacket(ip, port);
        }

        int endPacket() override
        {
            return peer_.endPacket();
        }

        size_t write(const uint8_t *buffer, size_t size) override
        {
            return peer_.write(buffer, size);
        }

        int parsePacket() override
        {
            return peer_.parsePacket();
        }

        int available() override
        {
            return peer_.available();
        }

        int read(uint8_t *buffer, size_t len) override
        {
            return peer_.read(buffer, len);
        }

        int peek() override
        {
            return peer_.peek();
        }

        void flush() override
        {
            peer_.flush();
        }

        IPAddress remoteIP() override
        {
            return peer_.remoteIP();
        }

        uint16_t remotePort() override
        {
            return peer_.remotePort();
        }

        /**
         * @brief Configures the underlying peer connection.
         *
         * Allows customization of the peer connection via a callback.
         *
         * @param callback A function to configure the peer.
         */
        void configureConnection(std::function<void(T *)> callback)
        {
            if (callback)
            {
                callback(&peer_);
            }
        }

    private:
        T peer_;
    };
} // namespace HttpServerAdvanced
