#pragma once

#include <Arduino.h>
#include "./Defines.h"
#include <memory>



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



} // namespace HttpServerAdvanced