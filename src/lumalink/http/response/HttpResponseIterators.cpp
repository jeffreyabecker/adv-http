#include "HttpResponseIterators.h"
#include "../core/HttpHeaderCollection.h"
#include <ctime>
#include <cstring>

namespace lumalink::http::response
{
    std::string getHeaderDateValue()
    {
        // This assumes the system time is correctly set using the NTP system
        struct tm tm_time;
        const time_t now = time(nullptr);
    #ifdef _WIN32
        gmtime_s(&tm_time, &now);
    #else
        gmtime_r(&now, &tm_time);
    #endif

        char buf[32];
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
        return std::string(buf);
    }

    void EnsureRequiredHeaders(HttpHeaderCollection &headers, std::ptrdiff_t body_size)
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
                headers.set(HttpHeaderNames::ContentLength, std::to_string(static_cast<size_t>(body_size)));
            }
            else
            {
                headers.set(HttpHeaderNames::TransferEncoding, "chunked");
            }
        }
    }
} // namespace lumalink::http::response

