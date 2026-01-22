#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include <LittleFS.h>
#define USE_WIFI 1
#include "./WifiSetup.h"
#include "./FSSetup.h"

WebServer server;
std::vector<std::pair<String, String>> getPinStates()
{
  std::vector<std::pair<String, String>> pinStates;
  for (int i = 0; i <= 20; i++)
  {
    pinStates.push_back({String(i), String(digitalRead(i))});
  }
  return pinStates;
}

Response onGetPin(HttpRequest &request, std::vector<String> &&params)
{
  int pin = params.size() > 0 ? params[0].toInt() : -1;
  if (pin < 0 || pin > 20)
  {
    return HttpResponse::create(HttpStatus::BadRequest(), String("Invalid pin number"), {HttpHeader::ContentType("text/plain")});
  }
  int value = digitalRead(pin);
  String response = String("Pin ") + pin + " value: " + String(value) + "\n";

  return HttpResponse::create(HttpStatus::Ok(), std::move(response), {HttpHeader::ContentType("text/plain")});
}
Response onSetPin(HttpRequest &request, std::vector<String> &&params, String && value)
{
    int pin = params.size() > 0 ? params[0].toInt() : -1;
    if (pin < 0 || pin > 20)
    {
      return HttpResponse::create(HttpStatus::BadRequest(), String("Invalid pin number"), {HttpHeader::ContentType("text/plain")});
    }
    String response = String("Set: Pin ") + pin + " value: " + String(value) + "\n";
    return HttpResponse::create(HttpStatus::Ok(), std::move(response), {HttpHeader::ContentType("text/plain")});
}
Response onRawBody(HttpRequest &request, RawBodyBuffer &&bodyData)
{
    String response = "Received raw body data:\n";
    response += String((const char *)bodyData.data(), bodyData.size()) + "\n";
    return HttpResponse::create(HttpStatus::Ok(), std::move(response), {HttpHeader::ContentType("text/plain")});
}
// Response onJsonBody(HttpRequest &request, JsonDocument &&jsonData)
// {
//     String response = "Received JSON body data:\n";
//     const char *sensor = jsonData["sensor"];
//     response += String(sensor) + "\n";
//     return HttpResponse::create(HttpStatus::Ok(), std::move(response), {HttpHeader::ContentType("application/json")});
// }

Response onFileUpload(HttpRequest &request, MultipartFormDataBuffer &&bodyData)
{
    String response = "Received multipart/form-data:\n";
    if (bodyData.status() == MultipartStatus::Completed)
    {
    }
    return HttpResponse::create(HttpStatus::Ok(), std::move(response), {HttpHeader::ContentType("text/plain")});
}
void setupWebServer(WebServer &server)
{
    auto handlers = server.cfg();
    auto basicAuthFilter = BasicAuth("admin", "password");
    // Serve static files from LittleFS /www at root. by default all request paths not starting with /api/ will be
    // mapped to files in /www
    handlers.use(StaticFiles(LittleFS));

    handlers.apply(CrossOriginRequestSharing());

    handlers.on("/api/pins/?", onGetPin);
    handlers.on<Buffered>("/api/pins/", onSetPin);
    handlers.on<RawBody>("/api/rawData", onRawBody);
    // handlers.on<Json>("/api/jsonData", onJsonBody);
}
void setupPins()
{
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 10; i++)
    {
      pinMode(i, INPUT);
    }
    for (int i = 10; i < 20; i++)
    {
      pinMode(i, OUTPUT);
    }
}
void setup()
{
    Serial.begin(115200);
    Serial.println("Raspberry Pi Pico initialized!");
    setupFilesystem();
    setupWiFi();
    setupPins();

    setupWebServer(server);
    server.begin();
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
}
