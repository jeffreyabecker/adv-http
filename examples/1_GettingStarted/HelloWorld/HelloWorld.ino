#include <Arduino.h>
#include <HttpServerAdvanced.h>

// Create a web server instance
WebServer server;

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
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting HelloWorld example...");

    // Configure the server
    auto handlers = server.cfg();
    
    // Register endpoints
    handlers.on<GetRequest>("/", helloWorldHandler);
    handlers.on<GetRequest>("/status", statusHandler);

    // Start the server on port 8080
    server.begin();
    Serial.println("Server started on port 8080");
}

void loop()
{
    delay(100);
}
