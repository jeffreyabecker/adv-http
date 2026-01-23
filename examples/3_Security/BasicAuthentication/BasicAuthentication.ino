#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include "../../../WifiSetup.h"
#include "../../../FSSetup.h"

WebServer server;

Response publicHandler(HttpRequest &request)
{
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "This is a public endpoint - no authentication required");
}

Response privateHandler(HttpRequest &request)
{
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "This is a private endpoint - Basic Auth required!");
}

Response adminHandler(HttpRequest &request)
{
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "Admin panel - high privilege access");
}

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting BasicAuthentication example...");

    setupWiFi();
    setupFilesystem();

    auto handlers = server.cfg();
    
    // Create an authenticator with username and password
    auto auth = BasicAuth("user", "password");
    
    // Public route - no authentication
    handlers.on<GetRequest>("/public", publicHandler);
    
    // Private route - protected with Basic Auth
    handlers.on<GetRequest>("/private", privateHandler).with(auth);
    
    // Admin route - different credentials
    auto adminAuth = BasicAuth("admin", "adminpass");
    handlers.on<GetRequest>("/admin", adminHandler).with(adminAuth);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("GET /public - open access");
    Serial.println("GET /private - requires Basic Auth (user:password)");
    Serial.println("GET /admin - requires Basic Auth (admin:adminpass)");

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
