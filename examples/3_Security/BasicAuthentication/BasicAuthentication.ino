#include <Arduino.h>
#include <HttpServerAdvanced.h>

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

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting BasicAuthentication example...");

    auto handlers = server.cfg();
    
    // Create an authenticator with username and password
    auto auth = BasicAuth("user", "password");
    
    // Public route - no authentication
    handlers.on<GetRequest>("/public", publicHandler);
    
    // Private route - protected with Basic Auth
    handlers.on<GetRequest>("/private", privateHandler).apply(auth);
    
    // Admin route - different credentials
    auto adminAuth = BasicAuth("admin", "adminpass");
    handlers.on<GetRequest>("/admin", adminHandler).apply(adminAuth);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("GET /public - open access");
    Serial.println("GET /private - requires Basic Auth (user:password)");
    Serial.println("GET /admin - requires Basic Auth (admin:adminpass)");
}

void loop()
{
    delay(100);
}
