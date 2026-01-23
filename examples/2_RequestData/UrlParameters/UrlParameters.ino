#include <Arduino.h>
#include <HttpServerAdvanced.h>

WebServer server;

Response userDetailsHandler(HttpRequest &request, std::vector<String> &&params)
{
    if (params.size() == 0)
    {
        return StringResponse::create(HttpStatus::BadRequest(), "text/plain", "User ID required");
    }
    
    String userId = params[0];
    String response = "User ID: " + userId;
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

Response ledControlHandler(HttpRequest &request, std::vector<String> &&params)
{
    if (params.size() == 0)
    {
        return StringResponse::create(HttpStatus::BadRequest(), "text/plain", "Command required");
    }
    
    String command = params[0];
    String response;
    
    if (command == "on")
    {
        digitalWrite(LED_BUILTIN, HIGH);
        response = "LED turned ON";
    }
    else if (command == "off")
    {
        digitalWrite(LED_BUILTIN, LOW);
        response = "LED turned OFF";
    }
    else
    {
        return StringResponse::create(HttpStatus::BadRequest(), "text/plain", "Invalid command");
    }
    
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

Response gpioSetHandler(HttpRequest &request, std::vector<String> &&params)
{
    if (params.size() < 2)
    {
        return StringResponse::create(HttpStatus::BadRequest(), "text/plain", "GPIO and value required");
    }
    
    int pin = params[0].toInt();
    int value = params[1].toInt();
    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
    
    String response = "GPIO " + String(pin) + " set to " + String(value);
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting UrlParameters example...");

    pinMode(LED_BUILTIN, OUTPUT);
    
    auto handlers = server.cfg();
    
    // Single parameter: /api/users/?
    handlers.on<GetRequest>("/api/users/?", userDetailsHandler);
    
    // Single parameter: /led/{command}
    handlers.on<GetRequest>("/led/?", ledControlHandler);
    
    // Multiple parameters: /gpio/{pin}/set/{value}
    handlers.on<GetRequest>("/gpio/?/set/?", gpioSetHandler);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("Try: /api/users/123, /led/on, /led/off, /gpio/5/set/1");
}

void loop()
{
    delay(100);
}
