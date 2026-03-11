#pragma once

#include <Arduino.h>
#include <cstddef>
#include "AsyncProcessing.h"

namespace NetInterface
{
    /**
     * @brief Interface for a network peer (e.g., UDP).
     *
     * This abstract class defines the interface for a peer that can send and receive
     * datagrams, join multicast groups, and manage connection status.
     */
    class IPeer
    /**
     * @class IPeer
     * @brief Abstract interface for network peer communication.
     *
     * This interface defines methods for initializing network connections,
     * handling multicast groups, sending and receiving packets, and managing
     * packet data. Implementations must provide mechanisms for starting and
     * stopping connections, processing incoming and outgoing packets, and
     * accessing remote host information.
     *
     * @note All methods are pure virtual unless otherwise specified.
     *
     * @see IPAddress
     * @see String
     */
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
        virtual void stop() = 0;

        /**
         * @brief Joins a multicast group and listens on the given port.
         *
         * @param interfaceAddr The local interface address.
         * @param multicast The multicast group address.
         * @param port The port number to listen on.
         * @return 1 if successful, 0 otherwise.
         */
        virtual uint8_t beginMulticast(IPAddress multicast, uint16_t port, IPAddress interfaceAddr) = 0;

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
         * @brief Writes data into the packet from a character buffer.
         * @param buffer Pointer to the character buffer containing data to write.
         * @param size Number of bytes to write.
         * @return The number of bytes written.
         */
        virtual size_t write(const char *buffer, size_t size)
        {
            return write(reinterpret_cast<const uint8_t *>(buffer), size);
        }
        /**
         * @brief Writes data into the packet from a String object.
         * @param data The String object containing data to write.
         * @return The number of bytes written.
         */
        virtual size_t write(const String &data)
        {
            return write(reinterpret_cast<const uint8_t *>(data.c_str()), data.length());
        }
        /**
         * @brief Writes data into the packet from a null-terminated character string.
         * @param data Pointer to the null-terminated character string to write.
         * @return The number of bytes written.
         */
        virtual size_t write(const char *data)
        {
            return write(reinterpret_cast<const uint8_t *>(data), strlen(data));
        }

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

        virtual size_t writeTo(const IPAddress &ip, uint16_t port, const uint8_t *buffer, size_t size)
        {
            beginPacket(ip, port);
            size_t bytesWritten = write(buffer, size);
            endPacket();
            return bytesWritten;
        }
    };

    class IAsyncPeer : public IPeer, public IHandleAsyncProcessing
    {
    public:
        using CallbackHandler = std::function<void(IAsyncPeer &, const IPAddress &, uint16_t)>;

        virtual bool listen(uint16_t port, CallbackHandler packetHandler) = 0;
        virtual bool listenMulticast(IPAddress multicast, uint16_t port, CallbackHandler packetHandler) = 0;

    };

} // namespace NetInterface
