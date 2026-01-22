#pragma once
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif
#include <Arduino.h>

const char *ssid = STASSID;
const char *password = STAPSK;
void setupNetwork()
{
    Serial.begin(115200);
#if USE_WIFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#elif USE_WIRED
    // Set up SPI pinout to match your HW
    SPI.setRX(0);
    SPI.setCS(1);
    SPI.setSCK(2);
    SPI.setTX(3);

    // Start the Ethernet port
    if (!eth.begin())
    {
        Serial.println("No wired Ethernet hardware detected. Check pinouts, wiring.");
        while (1)
        {
            delay(1000);
        }
    }

    // Wait for connection
    while (eth.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("IP address: ");
    Serial.println(eth.localIP());
#endif
}