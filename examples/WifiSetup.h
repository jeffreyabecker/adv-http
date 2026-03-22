#pragma once
#include <Arduino.h>
#include <WiFi.h>

#include "ExampleRuntime.h"

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        ExampleRuntime::delayMs(500);
        ExampleRuntime::print(".");
    }
    ExampleRuntime::println("Connected to WiFi");
    ExampleRuntime::print("IP Address: ");
    ExampleRuntime::println(WiFi.localIP());
}


