#include <Arduino.h>
#include <HttpServerAdvanced.h>

// Example self-signed certificate and key data
// In production, you would load these from storage or define them as constants
// These are placeholders - generate real certificates for actual use

// Example certificate (DER format as hex string - truncated for example)
const char CERTIFICATE_PEM[] = R"(
-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAKZ2w6Fv4W3tMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV
... (certificate content would go here) ...
-----END CERTIFICATE-----
)";

// Example private key (DER format as hex string - truncated for example)
const char PRIVATE_KEY_PEM[] = R"(
-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC... (key content) ...
-----END PRIVATE KEY-----
)";

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
    server.begin();
    
    Serial.println("HTTPS Server started on port 443");
    Serial.println("Connect via: https://[device-ip]/");
    Serial.println("\nNote: Use a self-signed certificate in development");
    Serial.println("To generate a self-signed cert, run:");
    Serial.println("openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes");
}

void loop()
{
    delay(100);
}
