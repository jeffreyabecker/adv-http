#pragma once

#include <cstdint>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
#endif

namespace HttpServerAdvanced
{
    using ClockMillis = std::uint32_t;

    class Clock
    {
    public:
        virtual ~Clock() = default;

        virtual ClockMillis nowMillis() const = 0;
    };

    class SystemClock final : public Clock
    {
    public:
        ClockMillis nowMillis() const override
        {
#ifdef ARDUINO
            return static_cast<ClockMillis>(millis());
#else
            using namespace std::chrono;
            return static_cast<ClockMillis>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
#endif
        }
    };

    class ManualClock final : public Clock
    {
    public:
        constexpr ManualClock() = default;

        explicit constexpr ManualClock(ClockMillis initialValue)
            : nowMillis_(initialValue)
        {
        }

        ClockMillis nowMillis() const override
        {
            return nowMillis_;
        }

        void setNowMillis(ClockMillis value)
        {
            nowMillis_ = value;
        }

        void advanceMillis(ClockMillis delta)
        {
            nowMillis_ += delta;
        }

    private:
        ClockMillis nowMillis_ = 0;
    };

    inline const Clock &DefaultClock()
    {
        static const SystemClock clock;
        return clock;
    }
}