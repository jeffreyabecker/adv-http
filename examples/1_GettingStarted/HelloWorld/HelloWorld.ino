#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include "../../WifiSetup.h"

// Create a web server instance
WebServer server;

volatile int setup0Done = 0;

// Handler for GET /
Response helloWorldHandler(HttpRequest &request)
{
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "Hello, World!");
}

// Handler for GET /status
Response statusHandler(HttpRequest &request)
{
    String status = "{\"status\":\"running\",\"uptime\":" + String(millis()) + "}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", status);
String status = "Status: running, Uptime: " + String(millis()) + " ms";
return StringResponse::create(HttpStatus::Ok(), "text/plain", status);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting HelloWorld example...");

    setupWiFi();

    // Configure the server
    auto handlers = server.cfg();
    
    // Register endpoints
    handlers.on<GetRequest>("/", helloWorldHandler);
    handlers.on<GetRequest>("/status", statusHandler);

    // Start the server on port 8080
    server.begin();
    Serial.println("Server started on port 8080");
    setup0Done = 1;
}

// Main loop to handle incoming requests. Bare metal requires that network code runs on core0
// for proper performance application code should run on core1
void loop()
{
    server.handleClient();
    delay(100);
}

void setup1(){
    while(setup0Done == 0){
        delay(100);
    }
}

void loop1(){
    // Your main application code goes here
    delay(100);
}
