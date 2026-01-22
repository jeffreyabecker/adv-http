#include <Arduino.h>
#include "../src/HttpServerAdvanced.h"
#include <LittleFS.h>

using namespace HttpServerAdvanced;

HttpServer server;

Response onGetPin(HttpRequest &request, std::vector<String> &&params)
{
  String pin = params.size() > 0 ? params[0] : "unknown";
  float value = request.uriView().queryView().get("mode").value_or("digital") == "analog" ? analogRead(pin.toInt()) : digitalRead(pin.toInt());
  String response = String("Pin ") + pin + " value: " + value + "\n";
  if (request.items().find("BasicAuth::Username") != request.items().end())
  {
    response += "Authenticated user: " + std::any_cast<String>(request.items().at("BasicAuth::Username")) + "\n";
  }
  return HttpResponse::create(HttpStatus::Ok(), response, {{"Content-Type", "text/plain"}});
}
Response onFormSubmit(HttpRequest &request, Form::PostBodyData &&bodyData)
{
  String response = "Received form data:\n";
  for (const auto &pair : bodyData.pairs())
  {
    response += pair.first + ": " + pair.second + "\n";
  }
  return HttpResponse::create(HttpStatus::Ok(), response, {{"Content-Type", "text/plain"}});
}
Response onRawBody(HttpRequest &request, RawBodyBuffer &&bodyData)
{
  String response = "Received raw body data:\n";
  response += String((const char *)bodyData.data(), bodyData.size()) + "\n";
  return HttpResponse::create(HttpStatus::Ok(), response, {{"Content-Type", "text/plain"}});
}
Response onJsonBody(HttpRequest &request, JsonDocument &&jsonData)
{
  String response = "Received JSON body data:\n";
  const char *sensor = jsonData["sensor"];
  response += String(sensor) + "\n";
  return HttpResponse::create(HttpStatus::Ok(), response, {{"Content-Type", "application/json"}});
}
void setupWebServer(WebServerBuilder &builder)
{
  // by default the static files are served at root "/" from "/www" in LittleFS
  // the path /api/* is excluded from static file serving by default
  // customize as needed
  builder.use(StaticFiles(LittleFS));
  builder.handlers().on<Request>("/api/pins/?", onGetPin);
  builder.handlers().on<Request>("/api/secure/pins/?", onGetPin).filter(BasicAuth("admin", "password"));
  builder.handlers().on<Form>("/api/formSubmit", onFormSubmit);
  builder.handlers().on<RawBody>("/api/rawData", onRawBody);
  builder.handlers().on<Json>("/api/jsonData", onJsonBody);
}
void setup()
{

  server.begin();

  server.use(CoreServices(setupWebServer));
}

void loop()
{
  server.handleClient();
}
