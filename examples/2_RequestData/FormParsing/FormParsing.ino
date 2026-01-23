#include <Arduino.h>
#include <HttpServerAdvanced.h>

WebServer server;

Response loginHandler(HttpRequest &request, PostBodyData &&formData)
{
    // Access form fields
    auto username = formData.get("username");
    auto password = formData.get("password");
    
    String response = "Login attempt:\n";
    
    if (username.has_value())
    {
        response += "Username: " + username.value() + "\n";
    }
    
    if (password.has_value())
    {
        response += "Password received\n";
    }
    
    // Simple validation
    if (username.has_value() && username.value() == "admin" && 
        password.has_value() && password.value() == "password")
    {
        return StringResponse::create(HttpStatus::Ok(), "text/plain", "Login successful!");
    }
    
    return StringResponse::create(HttpStatus::Unauthorized(), "text/plain", "Invalid credentials");
}

Response settingsHandler(HttpRequest &request, PostBodyData &&formData)
{
    String response = "Settings received:\n";
    
    auto brightness = formData.get("brightness");
    auto timeout = formData.get("timeout");
    
    if (brightness.has_value())
    {
        response += "Brightness: " + brightness.value() + "\n";
    }
    
    if (timeout.has_value())
    {
        response += "Timeout: " + timeout.value() + "\n";
    }
    
    return StringResponse::create(HttpStatus::Ok(), "text/plain", response);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nStarting FormParsing example...");

    auto handlers = server.cfg();
    
    // Form handlers receive PostBodyData (KeyValuePairView)
    handlers.on<Form>("/login", loginHandler);
    handlers.on<Form>("/settings", settingsHandler);

    server.begin();
    Serial.println("Server started on port 8080");
    Serial.println("POST to /login with username and password fields");
    Serial.println("POST to /settings with brightness and timeout fields");
}

void loop()
{
    delay(100);
}
