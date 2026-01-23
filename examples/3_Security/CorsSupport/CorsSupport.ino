#include <Arduino.h>
#include <HttpServerAdvanced.h>

WebServer server;

Response dataHandler(HttpRequest &request)
{
    String data = "{\"sensor\":\"temperature\",\"value\":23.5}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", data);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting CorsSupport example...");

    auto handlers = server.cfg();
    
    // Apply CORS to all responses
    handlers.apply(CrossOriginRequestSharing());
    
    // Endpoints that will have CORS headers added
    handlers.on<GetRequest>("/api/data", dataHandler);
    handlers.on<GetRequest>("/", [](HttpRequest &request) {
        return StringResponse::create(HttpStatus::Ok(), "text/plain", "CORS enabled - cross-origin requests allowed");
    });

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("All responses include CORS headers:");
    Serial.println("Access-Control-Allow-Origin: *");
    Serial.println("Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS");
    Serial.println("Access-Control-Allow-Headers: Content-Type, Authorization");
}

void loop()
{
    delay(100);
}
