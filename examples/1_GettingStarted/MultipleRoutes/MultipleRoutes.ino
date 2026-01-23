#include <Arduino.h>
#include <HttpServerAdvanced.h>

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
    uint32_t freeHeap = ESP.getFreeHeap();
    String json = "{\"freeHeap\":" + String(freeHeap) + ",\"unit\":\"bytes\"}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", json);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting MultipleRoutes example...");

    auto handlers = server.cfg();
    
    handlers.on<GetRequest>("/", welcomeHandler);
    handlers.on<GetRequest>("/api/time", timeHandler);
    handlers.on<GetRequest>("/api/heap", heapHandler);

    server.begin();
    Serial.println("Server started on port 8080");
}

void loop()
{
    delay(100);
}
