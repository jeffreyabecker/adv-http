#pragma once

#include <Arduino.h>
#include <cstdint>
#include <cstddef>
#include "ConnectionStatus.h"

namespace NetInterface
{
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
         * @brief Writes data to the client from a character buffer.
         * @param buf Pointer to the character buffer containing data to write.
         * @param size Number of bytes to write.
         */
        virtual std::size_t write(const char *buf, std::size_t size)
        {
            return write(reinterpret_cast<const uint8_t *>(buf), size);
        }
        /**
         * @brief Writes data to the client from a String object.
         * @param data The String object containing data to write.
         * @return The number of bytes actually written.
         */
        virtual std::size_t write(const char *data)
        {
            return write(reinterpret_cast<const uint8_t *>(data), strlen(data));
        }
        /**
         * @brief Writes data to the client from a String object.
         * @param data The String object containing data to write.
         * @return The number of bytes actually written.
         */
        virtual std::size_t write(const String &data)
        {
            return write(reinterpret_cast<const uint8_t *>(data.c_str()), data.length());
        }

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
         * @brief Reads data from the client into a character buffer.
         * @param rbuf Pointer to the character buffer where read data will be stored.
         *  @param size Maximum number of bytes to read.
         * @return The number of bytes actually read, or -1 on error.
         */
        virtual int read(char *rbuf, std::size_t size)
        {
            return read(reinterpret_cast<uint8_t *>(rbuf), size);
        }
        /**
         * @brief Reads data from the client into a String object.
         * @param maxLen Maximum number of bytes to read. If -1, reads all available bytes.
         * @return A String object containing the read data.
         */
        virtual String read(ssize_t maxLen = -1)
        {
            String result;
            int availableBytes = available();
            if (availableBytes > 0)
            {
                int bytesToRead = (maxLen < 0 || maxLen > availableBytes) ? availableBytes : maxLen;
                if (result.reserve(bytesToRead))
                {
                    int bytesRead = read(const_cast<char *>(result.c_str()), bytesToRead);
                    if (bytesRead > 0)
                    {
                        const_cast<char *>(result.c_str())[bytesRead] = '\0'; // Null-terminate the string
                    }
                }
            }
            return result;
        }

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

} // namespace NetInterface
