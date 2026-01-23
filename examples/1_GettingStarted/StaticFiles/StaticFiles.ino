#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include <LittleFS.h>

WebServer server;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting StaticFiles example...");

    // Initialize LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    
    Serial.println("LittleFS mounted successfully");

    auto handlers = server.cfg();
    
    // Serve static files from /www directory in LittleFS
    // GET /          → /www/index.html
    // GET /css/app.css → /www/css/app.css
    // GET /api/data → API route (not static)
    handlers.use(StaticFiles(LittleFS));
    
    // Add an API endpoint that won't be intercepted by static files
    handlers.on<GetRequest>("/api/data", [](HttpRequest &request) {
        String data = "{\"message\":\"This is an API endpoint, not a static file\"}";
        return StringResponse::create(HttpStatus::Ok(), "application/json", data);
    });

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("Static files are served from /www in LittleFS");
    Serial.println("MIME types are auto-detected from file extensions");
}

void loop()
{
    delay(100);
}
