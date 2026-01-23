#include <Arduino.h>
#include <HttpServerAdvanced.h>
#include "../../../WifiSetup.h"
#include "../../../FSSetup.h"


SecureHttpServerConfig<BR_KEYTYPE_RSA> httpsConfig(LittleFS, "/cert.pem", "/key.pem");
SecureWebServer server;

Response httpsRootHandler(HttpRequest &request)
{
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "HTTPS Server - Secure connection established!");
}

Response httpsDataHandler(HttpRequest &request)
{
    String data = "{\"secure\":true,\"protocol\":\"https\",\"port\":443}";
    return StringResponse::create(HttpStatus::Ok(), "application/json", data);
}

volatile int setup0Done = 0;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting HttpsServer example...");

    // Configure the secure server with certificate and key
    // Note: In a real implementation, you would:
    // 1. Generate a self-signed certificate using OpenSSL
    // 2. Load the certificate and key from persistent storage
    // 3. Or embed them as constants as shown above
    
    auto handlers = server.cfg();
    
    // Register HTTPS endpoints
    handlers.on<GetRequest>("/", httpsRootHandler);
    handlers.on<GetRequest>("/api/data", httpsDataHandler);
    
    // Add CORS support for secure connections
    handlers.apply(CrossOriginRequestSharing());

    // Start the HTTPS server on port 443
    // The server will use TLS 1.2+ for secure connections
    server.begin(httpsConfig);
    
    Serial.println("HTTPS Server started on port 443");
    Serial.println("Connect via: https://[device-ip]/");
    Serial.println("\nNote: Use a self-signed certificate in development");
    Serial.println("To generate a self-signed cert, run:");
    Serial.println("openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes");
    setup0Done = 1;
}
// Main loop to handle incoming HTTPS requests. Bare metal requires that network code runs on core0
// for proper performance application code should run on core1
void loop()
{
    server.handleClient();
}


void setup1(){
    while(setup0Done == 0){
        delay(100);
    }
}
void loop1(){
    // Your main application code goes here
    delay(100);
}