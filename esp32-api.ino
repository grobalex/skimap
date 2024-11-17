#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FastLED.h>
#include <ArduinoJson.h>

// Configuration for FastLED
#define LED_PIN     21
#define NUM_LEDS    20       
CRGB leds[NUM_LEDS];

unsigned long previousMillis = 0;
const long interval = 10000;

Preferences preferences;
WebServer server(80);

const char* AP_SSID = "Skimap setup"; 
const char* serverName = "http://198.211.104.146";
const int port = 6000;

// Static IP configuration
IPAddress local_IP(192, 168, 4, 1); 
IPAddress gateway(192, 168, 4, 1);   
IPAddress subnet(255, 255, 255, 0);    

// Flag to prevent restarting AP
bool apStarted = false;

// Start the configuration access point
void startConfigAP() {
    if (apStarted) return;

    Serial.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID)) {
        Serial.println("Failed to start Access Point!");
        return;
    }
    apStarted = true;

    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Failed to configure static IP for AP!");
        return;
    }

    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point IP: ");
    Serial.println(IP);

    server.on("/", HTTP_GET, []() {
        String html = "<html><body>";
        html += "<h1>Configure WiFi</h1>";
        html += "<form action='/save' method='POST'>";
        html += "SSID: <input type='text' name='ssid'><br>";
        html += "Password: <input type='password' name='password'><br>";
        html += "<input type='submit' value='Save'></form></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/save", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        preferences.putString("ssid", ssid);
        preferences.putString("password", password);

        server.send(200, "text/html", "<h1>Configuration Saved! Rebooting...</h1>");
        delay(2000);
        ESP.restart();
    });

    server.begin();
    Serial.println("Web server started.");
}

// Connect to WiFi
void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    String ssid = preferences.getString("ssid");
    String password = preferences.getString("password");

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi...");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
        apStarted = false;
        server.begin();
    } else {
        Serial.println("Failed to connect to WiFi. Starting AP once.");
        apStarted = true;
    }
}

// Main function to send HTTP request and update LEDs
void runMainScript() {
    HTTPClient http;
    String url = String(serverName) + ":" + String(port) + "/status";
    Serial.println("Sending request to: " + url);

    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        String responseBody = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, responseBody);

        if (error) {
            Serial.println("Failed to parse JSON: " + String(error.c_str()));
        } else {
            for (JsonObject item : doc.as<JsonArray>()) {
                int ledPosition = item["LED"].as<int>() - 1;
                String status = item["status"].as<String>();

                if (ledPosition >= 0 && ledPosition < NUM_LEDS) {
                    if (status == "OPEN") {
                        leds[ledPosition] = CRGB::Green;
                    } else if (status == "HOLD") {
                        leds[ledPosition] = CRGB::Yellow;
                    } else if (status == "CLOSED") {
                        leds[ledPosition] = CRGB::Red;
                    } else {
                        leds[ledPosition] = CRGB::Black;
                    }
                }
            }
            FastLED.show();
        }
    } else {
        Serial.println("Error in HTTP request. Response code: " + String(httpResponseCode));
    }

    http.end();
}

void setup() {
    Serial.begin(9600);
    FastLED.addLeds<WS2811, LED_PIN , RGB>(leds, NUM_LEDS);
    preferences.begin("wifi-config", false);

    if (preferences.getString("ssid", "") == "") {
        Serial.println("No SSID found, starting Config AP...");
        startConfigAP();
    } else {
        connectToWiFi();
    }
}

void loop() {
    if (WiFi.status() != WL_CONNECTED && !apStarted) {
        Serial.println("WiFi disconnected, starting Config AP...");
        startConfigAP();
    }

    server.handleClient();

    unsigned long currentMillis = millis();
    if (WiFi.status() == WL_CONNECTED && (currentMillis - previousMillis >= interval)) {
        previousMillis = currentMillis;
        runMainScript();
    }
}
