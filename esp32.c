#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <FastLED.h>

// Configuration for FastLED
#define LED_PIN     13
#define NUM_LEDS    20       
CRGB leds[NUM_LEDS];         

Preferences preferences;
WebServer server(80);

const char* AP_SSID = "Skimap setup"; 
const char* apiEndpoint = ""; 

// Static IP configuration
IPAddress local_IP(192, 168, 4, 1); 
IPAddress gateway(192, 168, 4, 1);   
IPAddress subnet(255, 255, 255, 0);    

// Flag to prevent restarting AP
bool apStarted = false; 

void setup() {
    Serial.begin(9600);
    preferences.begin("wifi-config", false);
    bool resetFlag = preferences.getBool("resetFlag", false);

    if (resetFlag) {
        Serial.println("Detected reset. Clearing preferences...");
        preferences.clear();
        
        // Reset the flag to avoid clearing on every restart
        preferences.putBool("resetFlag", false);

        Serial.println("Preferences cleared. Restarting...");
        delay(1000);
        ESP.restart();  // Restart the ESP32
    } else {
        // reset flag so that preferences are cleared on restart
        preferences.putBool("resetFlag", true);
        Serial.println("Preferences will be cleared on the next reset.");
    }
    pinMode(LED_PIN, OUTPUT);
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.clear(); 

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
}

// Start the configuration access point
void startConfigAP() {
    if (apStarted) return; // Avoid re-entering AP setup

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
        Serial.println("Serving configuration page...");
        String html = "<html><body>";
        html += "<h1>Configure WiFi and Select Ski Resort</h1>";
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
        apStarted = false; // Reset flag, as AP is not needed now
        server.begin(); 
        cycleColors();
    } else {
        Serial.println("Failed to connect to WiFi. Starting AP once.");
        apStarted = true; // Set flag to true, so the AP does not restart endlessly
    }
}

// Main script to handle API calls
//// NOT USED currently
void runMainScript() {
    HTTPClient http;
    http.begin(String(apiEndpoint) + "?resort=");

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("API Response: " + payload);

        FastLED.clear();
    }

    http.end();
}

// Function to cycle through colors on the LED strip
void cycleColors() {
    static uint8_t hue = 0; 
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hue++, 255, 255); 
    }
    FastLED.show();
    delay(20); 
}
