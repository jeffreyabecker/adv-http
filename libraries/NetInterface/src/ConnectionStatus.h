#pragma once

#include <Arduino.h>

namespace NetInterface
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

} // namespace NetInterface
