#include "HttpResponseIterators.h"
#include "./core/HttpHeaderCollection.h"
#include <ctime>
#include <sys/time.h>
#include <cstring>

namespace HttpServerAdvanced
{
    String getHeaderDateValue()
    {
        // This assumes the system time is correctly set using the NTP system
        struct tm tm_time;
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        time_t now = tv.tv_sec;
        gmtime_r(&now, &tm_time);

        char buf[32];
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
        return String(buf);
    }

    void EnsureRequiredHeaders(HttpHeaderCollection &headers, ssize_t body_size)
    {
        // Date header (RFC 7231 section 7.1.1.2 - MUST be sent by origin servers)
        if (!headers.exists(HttpHeaderNames::Date))
        {
            headers.set(HttpHeaderNames::Date, getHeaderDateValue());
        }
        if (!headers.exists(HttpHeaderNames::Server))
        {
            headers.set(HttpHeaderNames::Server, "Arduino-Pico");
        }
        if (!headers.exists(HttpHeaderNames::ContentType))
        {
            headers.set(HttpHeaderNames::ContentType, "text/plain");
        }
        if (!headers.exists(HttpHeaderNames::Connection))
        {
            headers.set(HttpHeaderNames::Connection, "close");
        }
        if(!headers.exists(HttpHeaderNames::ContentLength) && 
           !headers.exists(HttpHeaderNames::TransferEncoding))
        {
            if (body_size >= 0)
            {
                headers.set(HttpHeaderNames::ContentLength, String(static_cast<size_t>(body_size)));
            }
            else
            {
                headers.set(HttpHeaderNames::TransferEncoding, "chunked");
            }
        }
    }
} // namespace HttpServerAdvanced
