#include <Arduino.h>
#include <NetClient.h>

// Platform-specific includes
#ifdef ESP32
#include <WiFi.h>
#include <WiFiServer.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#elif defined(RASPBERRY_PI_PICO)
#include <WiFi.h>
#include <WiFiServer.h>
#endif

using namespace NetInterface;

const char* ssid = "your-wifi-ssid";
const char* password = "your-wifi-password";
const uint16_t serverPort = 80;

std::unique_ptr<ServerImpl<WiFiServer>> server;

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

    // Create server wrapper
    server = std::make_unique<ServerImpl<WiFiServer>>(serverPort);
    server->begin();
    
    Serial.print("Server started on port: ");
    Serial.println(server->port());
}

void loop() {
    // Check for client connections
    auto client = server->accept();
    
    if (client) {
        Serial.println("New client connected");
        Serial.print("Client IP: ");
        Serial.println(client->remoteIP());
        Serial.print("Client Port: ");
        Serial.println(client->remotePort());
        
        // Simple response
        const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello from NetInterface!\r\n";
        client->write((const uint8_t*)response, strlen(response));
        client->flush();
        
        delay(10);
        client->stop();
        Serial.println("Client disconnected");
    }
    
    delay(100);
}