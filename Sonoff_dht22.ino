#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>;

#define DHTPIN 14     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value


struct Led {
    byte id;
    byte gpio;
    byte status;
} led_resource;

const char* wifi_ssid = "********";
const char* wifi_passwd = "********";

int relay = 12;
bool ledValue = false;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

void init_led_resource()
{
    led_resource.id = 1;
    led_resource.gpio = 0;
    led_resource.status = LOW;
}

int init_wifi() {
    int retries = 0;

    Serial.println("Connecting to WiFi AP..........");

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    IPAddress ip(172,20,22,88);   
    IPAddress gateway(172,20,22,254);   
    IPAddress subnet(255,255,255,0);   
    WiFi.config(ip, gateway, subnet);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}

void getFromDHT() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];
    
    temp= dht.readTemperature();
    hum = dht.readHumidity();
    
    jsonObj["temperature"] = temp;
    jsonObj["humidity"] = hum;
    jsonObj["led_status"] = ledValue;
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
    
}

void ledOn() {
    ledValue = true;
    digitalWrite(relay, HIGH);
  
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];

    jsonObj["led_status"] = ledValue;
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void ledOff() {
    ledValue = false;
    digitalWrite(relay, LOW);
  
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];

    jsonObj["led_status"] = ledValue;
    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/getFromDHT", HTTP_GET, getFromDHT);
    http_rest_server.on("/ledOn", HTTP_GET, ledOn);
    http_rest_server.on("/ledOff", HTTP_GET, ledOff);
}

void setup(void) {
    Serial.begin(115200);
    pinMode(relay, OUTPUT);
    digitalWrite(relay, LOW);
    dht.begin();
    init_led_resource();
    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}

void loop(void) {
    http_rest_server.handleClient();
}
