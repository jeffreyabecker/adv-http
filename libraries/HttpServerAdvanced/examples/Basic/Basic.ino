#include "./WifiSetup.h"
#include <HTTPServer.h>
#include <LittleFS.h>

HttpServer WebServer;

Response onHello(HttpContext &context)
{
    return HttpResponse::create(HttpStatus::Ok(), "text/plain", "Hello World\n");
}

Response onPinRequest(HttpContext &context, std::vector<String> &&pathParams)
{
    if (pathParams.size() > 0)
    {
        int pin = pathParams[0].toInt();
        int value = digitalRead(pin); // Just an example operation
        String response = "Requested pin: " + String(pin) + ". Value: " + String(value) + "\n";
        return HttpResponse::create(HttpStatus::Ok(), "text/plain", response);
    }
    else
    {
        return HttpResponse::create(HttpStatus::BadRequest(), "text/plain", "Pin not specified\n");
    }
}
Response onGreeting(HttpContext &context, PostBodyData &&pathParams)
{
    String name = pathParams.get("name").value_or("Guest");
    String response = "Hello, " + name + "!\n";
    return HttpResponse::create(HttpStatus::Ok(), "text/plain", response);
}

Response onEcho(HttpContext &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength){
    std::vector<uint8_t> buffer;
    if(recievedLength ==0){
        buffer.reserve(contentLength);
    };
    buffer.insert(buffer.end(), chunk, chunk + chunkLength);
    String response = "Received " + String(recievedLength) + " of " + String(contentLength) + " bytes\n";
    response += "Chunk (" + String(chunkLength) + " bytes): ";
    response += String(reinterpret_cast<const char *>(chunk), chunkLength) + "\n";
    return HttpResponse::create(HttpStatus::Ok(), "text/plain", response);
}
// Response onFileUpload(HttpContext &context, PostBodyData &&postData)
// {
//     auto fileOpt = postData.files().get("uploadFile");
//     if (fileOpt.has_value())
//     {
//         auto &fileData = fileOpt.value();
//         auto contentType = fileData.contentType();
//         String response = "Received file: " + fileData.filename()+" " + contentType + " (" + String(fileData.length()) + " bytes)\n";
//         return HttpResponse::create(HttpStatus::Ok(), "text/plain", response);
//     }
//     else
//     {
//         return HttpResponse::create(HttpStatus::BadRequest(), "text/plain", "No file uploaded\n");
//     }
// }

void setup()
{
    setupWiFi();

    WebServer.use(CoreServices([](WebServerBuilder &b)
                               {
        // Set up static file serving from LittleFS at root. The default configuration
        // serves files from /www in LittleFS. Requests to "/" will serve "/www/index.html".
        // By default apis are exposed under /api/. A builder callback can be supplied to further configure
        // the static file serving behavior.
        b.use(StaticFiles(LittleFS));
        auto authentication = BasicAuth("admin", "password", "My Realm");
        b.handlers().on<Request>("/api/hello", onHello);
        b.handlers().on<Request>("/api/secure", onHello).apply(authentication);
        b.handlers().on<RawBody>("/api/echo", onEcho);
        b.handlers().on<Request>("/api/pins/?", onPinRequest); 
        b.handlers().on<Form>("/api/greet", onGreeting);
        //b.handlers().on<Form>("/api/file", onFileUpload);
    }));
    WebServer.begin();
}

void loop()
{
    WebServer.handleClient();
}