#pragma once

#include "IPeer.h"
#include <type_traits>
#include <functional>
#include <utility>
namespace NetInterface
{
    /**
     * @brief Implementation of PeerWrapper for a specific Peer type T.
     *
     * T is required to implement the peer interface through SFINAE.
     *
     * @tparam T The peer type to wrap.
     */
    template <typename T>
    class PeerImpl : public IRawPeer
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
        ~PeerImpl() override
        {
            if (peer_)
            {
                peer_.stop();
            }
        }

        uint8_t begin(uint16_t port) override
        {
            return peer_.begin(port);
        }

        uint8_t beginMulticast(IPAddress multicast, uint16_t port) override
        {
            return peer_.beginMulticast(multicast, port);
        }

        void stop() override
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

    template <typename TPeer, typename = std::enable_if_t<std::is_convertible_v<TPeer, IPeer>>>
    class AsyncPeerImpl : public TPeer, public IAsyncPeer
    {
    public:
        using TPeer::TPeer; // Inherit constructors
        /**
         * @brief Destructor for AsyncPeerImpl.
         */
        ~AsyncPeerImpl() override = default;

        bool listen(uint16_t port, CallbackHandler packetHandler) override
        {
            packetHandler_ = packetHandler;
            return TPeer::begin(port) != 0;
        }
        bool listenMulticast(IPAddress multicast, uint16_t port, CallbackHandler packetHandler) override
        {
            packetHandler_ = packetHandler;
            return TPeer::beginMulticast(multicast, port) != 0;
        }
        void handleIncomingRequests() override
        {
            int packetSize = TPeer::parsePacket();
            if (packetSize > 0 && packetHandler_)
            {
                IPAddress remoteIp = TPeer::remoteIP();
                uint16_t remotePort = TPeer::remotePort();
                packetHandler_(*this, remoteIp, remotePort);
            }
        }

    private:
        CallbackHandler packetHandler_;
    };
} // namespace NetInterface
