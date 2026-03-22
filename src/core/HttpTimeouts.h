#pragma once
#include "../compat/Clock.h"
#include "Defines.h"

namespace HttpServerAdvanced
{

    /// @brief Timeouts related to HTTP Server operations
    ///
    /// This class manages three types of timeouts for HTTP request processing:
    ///
    /// 1. Total Request Length: Maximum time allowed for the entire request lifecycle
    ///    (from initial connection to request completion)
    ///
    /// 2. Action Timeout: Maximum time allowed between successive parser callbacks
    ///    or processing actions without any progress
    ///
    /// 3. Read Timeout: Maximum time allowed to wait for incoming data from the client
    ///    when the socket buffer is empty
    ///
    /// All timeout values are specified in milliseconds.
    class HttpTimeouts
    {
    private:
        /// @brief Maximum total time (ms) allowed for complete request processing
        /// @details Default: PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS
        Compat::ClockMillis _totalRequestLengthMs = PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS;

        /// @brief Maximum time (ms) between processing actions
        /// @details Default: PIPELINE_ACTIVITY_TIMEOUT
        Compat::ClockMillis _activityTimeout = PIPELINE_ACTIVITY_TIMEOUT;

        /// @brief Maximum time (ms) to wait for incoming data
        /// @details Default: PIPELINE_READ_TIMEOUT
        Compat::ClockMillis _readTimeout = PIPELINE_READ_TIMEOUT;

    public:
        // Getters

        /// @brief Get the maximum total request processing time
        /// @return Timeout value in milliseconds
        Compat::ClockMillis getTotalRequestLengthMs() const { return _totalRequestLengthMs; }

        /// @brief Get the action timeout (time between processing steps)
        /// @return Timeout value in milliseconds
        Compat::ClockMillis getActivityTimeout() const { return _activityTimeout; }

        /// @brief Get the read timeout (waiting for client data)
        /// @return Timeout value in milliseconds
        Compat::ClockMillis getReadTimeout() const { return _readTimeout; }

        // Setters

        /// @brief Set the maximum total request processing time
        /// @param timeout Timeout value in milliseconds
        void setTotalRequestLengthMs(Compat::ClockMillis timeout) { _totalRequestLengthMs = timeout; }

        /// @brief Set the action timeout (time between processing steps)
        /// @param timeout Timeout value in milliseconds
        void setActivityTimeout(Compat::ClockMillis timeout) { _activityTimeout = timeout; }

        /// @brief Set the read timeout (waiting for client data)
        /// @param timeout Timeout value in milliseconds
        void setReadTimeout(Compat::ClockMillis timeout) { _readTimeout = timeout; }
    };
}
