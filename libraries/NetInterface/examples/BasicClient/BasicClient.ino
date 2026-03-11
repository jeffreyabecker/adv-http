#include <Arduino.h>
#include <NetClient.h>

// Platform-specific includes
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#elif defined(RASPBERRY_PI_PICO)
#include <WiFi.h>
#include <WiFiClient.h>
#endif

using namespace NetInterface;

const char* ssid = "your-wifi-ssid";
const char* password = "your-wifi-password";

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Create a client wrapper
#ifdef ESP32
    WiFiClient wifiClient;
    auto client = std::make_unique<ClientImpl<WiFiClient>>(wifiClient);
#elif defined(ESP8266)
    WiFiClient wifiClient;
    auto client = std::make_unique<ClientImpl<WiFiClient>>(wifiClient);
#elif defined(RASPBERRY_PI_PICO)
    WiFiClient wifiClient;
    auto client = std::make_unique<ClientImpl<WiFiClient>>(wifiClient);
#endif

    // Example usage of the client
    Serial.println("Client created successfully");
    
    // Test connection status
    Serial.print("Connection status: ");
    Serial.println(static_cast<int>(client->status()));
    
    Serial.print("Is connected: ");
    Serial.println(client->connected());
}

void loop() {
    // Main loop - could handle client operations here
    delay(1000);
}