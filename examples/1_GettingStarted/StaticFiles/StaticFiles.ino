#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include <LittleFS.h>
#include "../../WifiSetup.h"
#include "../../FSSetup.h"

WebServer server;

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting StaticFiles example...");

    setupWiFi();
    setupFilesystem();

    auto handlers = server.cfg();
    
    // Serve static files from /www directory in LittleFS
    // GET /          → /www/index.html
    // GET /css/app.css → /www/css/app.css
    // GET /api/data → API route (not static)
    handlers.use(StaticFiles(LittleFS));
    
    // Add an API endpoint that won't be intercepted by static files
    handlers.on<GetRequest>("/api/data", [](HttpRequest &request) {
        unsigned long uptime = millis();
        String data = "{\"message\":\"This is an API endpoint, not a static file\",\"uptime\":" + String(uptime) + "}";
        return StringResponse::create(HttpStatus::Ok(), "application/json", data);
    });

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("Static files are served from /www in LittleFS");
    Serial.println("MIME types are auto-detected from file extensions");

    setup0Done = 1;
}

// Main loop to handle incoming requests. Bare metal requires that network code runs on core0
// for proper performance application code should run on core1
void loop()
{
    server.handleClient();
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
