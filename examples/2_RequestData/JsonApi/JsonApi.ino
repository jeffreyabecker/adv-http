#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include <ArduinoJson.h>

WebServer server;

Response configGetHandler(HttpRequest &request)
{
    // Return current configuration as JSON
    String config = "{\"wifi\":\"enabled\",\"port\":8080,\"debug\":true}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", config);
}

Response ledControlJsonHandler(HttpRequest &request)
{
    // Example JSON response for LED control
    String response = "{\"led\":\"controlled\",\"status\":\"on\"}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", response);
}

Response configPostHandler(HttpRequest &request, PostBodyData &&formData)
{
    // Simple echo back of received parameters as JSON
    String response = "{\"received\":true,\"message\":\"Configuration updated\"}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", response);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting JsonApi example...");

    auto handlers = server.cfg();
    
    // GET current config
    handlers.on<GetRequest>("/api/config", configGetHandler);
    
    // POST to update config (using Form for simplicity; true JSON parsing would use dedicated handler)
    handlers.on<Form>("/api/config", configPostHandler);
    
    // LED control
    handlers.on<GetRequest>("/api/led", ledControlJsonHandler);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("GET /api/config - retrieve configuration");
    Serial.println("GET /api/led - get LED status");
}

void loop()
{
    delay(100);
}
