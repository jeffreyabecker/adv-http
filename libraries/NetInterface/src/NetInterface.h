#pragma once

#include "ConnectionStatus.h"
#include "IClient.h"
#include "IServer.h"
#include "IPeer.h"
#include "ClientImpl.h"
#include "ServerImpl.h"
#include "PeerImpl.h"

#include <WiFi.h>

namespace NetInterface
{
    using NetServer = ServerImpl<WiFiServer>;
    using NetClient = ClientImpl<WiFiClient>;
    using NetPeer = PeerImpl<WiFiUDP>;
    using NetPeerAsync = AsyncPeerImpl<WiFiUDP>;
    using NetServerAsync = AsyncServerImpl<WiFiServer>;
} // namespace NetInterface

using NetServer = NetInterface::NetServer;
using NetClient = NetInterface::NetClient;
using NetPeer = NetInterface::NetPeer;
using NetPeerAsync = NetInterface::NetPeerAsync;
using NetServerAsync = NetInterface::NetServerAsync;