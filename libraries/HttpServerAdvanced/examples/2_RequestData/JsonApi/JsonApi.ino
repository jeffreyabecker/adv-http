#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include <ArduinoJson.h>
#include "../../../WifiSetup.h"
#include "../../../FSSetup.h"

WebServer server;

Response configGetHandler(HttpRequest &request)
{
    JsonDocument doc;
    doc["wifi"] = "enabled";
    doc["localIp"] = request.localIP().toString();
    doc["localPort"] = request.localPort();
    doc["debug"] = true;
    // Return current configuration as JSON
    return JsonResponse::create(HttpStatus::Ok(), doc);
}
JsonDocument getCurrentPin(){
    int pinStatus = digitalRead(LED_BUILTIN);
    JsonDocument doc;
    doc["led"] = "controlled";
    doc["status"] = pinStatus == HIGH ? "on" : "off";
    return doc;
}

Response ledControlJsonHandler(HttpRequest &request)
{
        // Example JSON response for LED control
    JsonDocument doc = getCurrentPin();
    return JsonResponse::create(HttpStatus::Ok(), doc);
}

Response ledControlPostHandler(HttpRequest &request, JsonDocument &&body)
{
    if (body.containsKey("led"))
    {
        String ledState = body["led"];
        if (ledState == "on")
        {
            digitalWrite(LED_BUILTIN, HIGH);
        }
        else if (ledState == "off")
        {
            digitalWrite(LED_BUILTIN, LOW);
        }
    }

    JsonDocument doc = getCurrentPin();
    return JsonResponse::create(HttpStatus::Ok(), doc);
}

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting JsonApi example...");

    setupWiFi();
    setupFilesystem();
    pinMode(LED_BUILTIN, OUTPUT);

    auto handlers = server.cfg();

    // GET current config
    handlers.on<GetRequest>("/api/config", configGetHandler);

    // POST to update config (using Form for simplicity; true JSON parsing would use dedicated handler)
    handlers.on<Json>("/api/led", ledControlPostHandler);

    // LED control
    handlers.on<GetRequest>("/api/led", ledControlJsonHandler);

    server.begin();
    Serial.println("Server started on port " + String(server.localPort()));
    Serial.println("GET /api/config - retrieve configuration");
    Serial.println("GET /api/led - get LED status");
    Serial.println("POST /api/led {\"led\":\"on\"} - turn LED on");
    Serial.println("POST /api/led {\"led\":\"off\"} - turn LED off");

    setup0Done = 1;
}

// Main loop to handle incoming requests. Bare metal requires that network code runs on core0
// for proper performance application code should run on core1
void loop()
{
    server.handleClient();
    delay(100);
}

void setup1()
{
    while (setup0Done == 0)
    {
        delay(100);
    }
}
void loop1()
{
    // Your main application code goes here
    delay(100);
}
