#include <Arduino.h>
#include <HttpServerAdvanced.h>

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

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting QueryStrings example...");

    auto handlers = server.cfg();
    
    handlers.on<GetRequest>("/search", searchHandler);
    handlers.on<GetRequest>("/config", configHandler);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("Try: /search?q=hello&limit=10");
    Serial.println("Try: /config?wifi=mySSID&pass=myPassword");
}

void loop()
{
    delay(100);
}
