#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include "../../WifiSetup.h"
#include "../../FSSetup.h"

WebServer server;

Response welcomeHandler(HttpRequest &request)
{
    String html = "<html><body><h1>Welcome!</h1><p>Multi-route example server</p></body></html>";
    return StringResponse::create(HttpStatus::Ok(), "text/html", html);
}

Response timeHandler(HttpRequest &request)
{
    String json = "{\"timestamp\":" + String(millis()) + ",\"message\":\"Current server uptime in ms\"}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", json);
}

Response heapHandler(HttpRequest &request)
{
    uint32_t freeHeap = rp2040.getFreeHeap();
    String json = "{\"freeHeap\":" + String(freeHeap) + ",\"unit\":\"bytes\"}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", json);
}

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting MultipleRoutes example...");

    setupWiFi();
    setupFilesystem();

    auto handlers = server.cfg();
    
    handlers.on<GetRequest>("/", welcomeHandler);
    handlers.on<GetRequest>("/api/time", timeHandler);
    handlers.on<GetRequest>("/api/heap", heapHandler);

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
