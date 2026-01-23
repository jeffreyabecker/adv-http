#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include "../../../WifiSetup.h"
#include "../../../FSSetup.h"

WebServer server;

Response searchHandler(HttpRequest &request)
{
    // Parse query string: ?q=hello&limit=10
    String uri = request.url();
    String response = "Query parameters received:\n";
    
    // Simple query parsing example
    int qPos = uri.indexOf('?');
    if (qPos != -1)
    {
        String query = uri.substring(qPos + 1);
        response += "Raw query: " + query + "\n";
    }
    
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

Response configHandler(HttpRequest &request)
{
    // Query string: ?wifi=ssid&pass=pw
    String uri = request.url();
    String response = "Configuration request:\n";
    
    int qPos = uri.indexOf('?');
    if (qPos != -1)
    {
        String query = uri.substring(qPos + 1);
        response += "Query: " + query + "\n";
        response += "Parse parameters as needed\n";
    }
    else
    {
        response += "No configuration parameters provided\n";
    }
    
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting QueryStrings example...");

    setupWiFi();
    setupFilesystem();

    auto handlers = server.cfg();
    
    handlers.on<GetRequest>("/search", searchHandler);
    handlers.on<GetRequest>("/config", configHandler);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("Try: /search?q=hello&limit=10");
    Serial.println("Try: /config?wifi=mySSID&pass=myPassword");

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
