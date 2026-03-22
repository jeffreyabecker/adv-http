#pragma once

#include <Arduino.h>

namespace ExampleRuntime
{
    inline void delayMs(unsigned long durationMs)
    {
        delay(durationMs);
    }

    template <typename TValue>
    inline void print(const TValue &value)
    {
        Serial.print(value);
    }

    template <typename TValue>
    inline void println(const TValue &value)
    {
        Serial.println(value);
    }
}