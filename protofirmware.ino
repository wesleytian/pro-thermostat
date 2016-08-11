#include "DHT.h"
#include "font.h"
#include <Wire.h>
#include "SSD1306.h"
#include <RCSwitch.h>
#include <SimpleTimer.h>

 // This part is for Ethernet stuff
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <BlynkSimpleESP8266.h>

RCSwitch mySwitch = RCSwitch();

// what digital pin we're connected to
#define DHTPIN 14     
 // Enables Serial Monitor
#define BLYNK_PRINT Serial
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

SSD1306  display(0x3c, SCL, SDA);

DHT dht(DHTPIN, DHTTYPE);
int onSignal = 1381827;
int offSignal = 1381836;
int currentSignal = 0;

bool state = false;
long last = -180000;

float tempSetting = 90;
float previousTempSetting;
unsigned long timeSinceStart;

// the timer object
SimpleTimer timer;

char auth[] = "cc7e38a7f1024960aca086a910777bcb"; // Put your Auth Token here. (see Step 3 above)
const char* ssid = "freefly";
const char* password = "lrtsucks";
ESP8266WebServer server(80);
const int led = 2;

// There is a Widget that WRITEs data to V1 
BLYNK_WRITE(V5) {
  previousTempSetting = tempSetting;
  tempSetting = param.asInt();
}

float h;
float t;
float f;
float previoush;
float previoust;
float previousf;
float hif;

// a function to be executed periodically
void repeatMe() {
  float highTemp = tempSetting + 0.5;
  float lowTemp = tempSetting - 0.5;

  h = dht.readHumidity();
  t = dht.readTemperature();
  f = dht.readTemperature(true);
  
  float hic = dht.computeHeatIndex(t, h, false); // Compute heat index in Celsius (isFahreheit = false)

  display.clear();
  if (!isnan(f) && !isnan(h)) {
      Serial.print("[ ");
  Serial.print(millis() / 1000);
  Serial.print(" secs ]..... \n");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0,0,String(h) + " %");
    previoush = h;

    display.drawString(0,24,String(f) + " °F");
    previousf = f;

    hif = dht.computeHeatIndex(f, h); // Compute heat index in Fahrenheit (the default)
    display.drawString(0,48,String(hif) + " °F");
  } 
  else if (!isnan(h) && isnan(f)) {
      Serial.print("[ ");
  Serial.print(millis() / 1000);
  Serial.print(" secs ].....");
    Serial.println("Failed to read temperature from DHT sensor.");
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.drawString(0,0,String(h) + " %");
    previoush = h;

    display.drawString(0,24,String(previousf) + " °F");

    hif = dht.computeHeatIndex(previousf, h); // Compute heat index in Fahrenheit (the default)
    display.drawString(0,48,String(hif) + " °F");
  }
  else if (!isnan(f) && isnan(h)) {
      Serial.print("[ ");
  Serial.print(millis() / 1000);
  Serial.print(" secs ].....");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    Serial.println("Failed to read humidity from DHT sensor.");

    display.drawString(0,0,String(previoush) + " %");

    display.drawString(0,24,String(f) + " °F");
    previousf = f;

    hif = dht.computeHeatIndex(f, previoush); // Compute heat index in Fahrenheit (the default)
    display.drawString(0,48,String(hif) + " °F");
  }
  else if (isnan(h) && isnan(f)) {
      Serial.print("[ ");
  Serial.print(millis() / 1000);
  Serial.print(" secs ].....");
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.drawString(0,0,String(previoush) + " %");

    Serial.println("Failed to read temperature and humidity from DHT sensor.");
    display.drawString(0,24,String(previousf) + " °F");

    hif = dht.computeHeatIndex(previousf, previoush); // Compute heat index in Fahrenheit (the default)
    display.drawString(0,48,String(hif) + " °F");
  }

  //Switch control.
  if ((state == false) && (millis() > last + 180000) && (f > highTemp)) {
    currentSignal = onSignal;
    mySwitch.send(currentSignal, 24);
    delay(2000);
    last = millis();
    state = true;
    Serial.println("ON");
  }

  else if ((state == true) && (f < lowTemp)) {
    currentSignal = offSignal;
    mySwitch.send(currentSignal, 24);
    delay(2000);
    state = false;
    Serial.println("OFF");
  }

  Blynk.virtualWrite(V0, h);
  Blynk.virtualWrite(V1, f);
  Blynk.virtualWrite(V2, hif);

  if(tempSetting != previousTempSetting) {
    Serial.print("Thermostat temperature set to: ");
    Serial.println(tempSetting);
    previousTempSetting = tempSetting;
  }
  display.display();
}

void setup() {
  Serial.begin(115200);

  Serial.println("");
  WiFi.begin(ssid, password);
  // digitalWrite(led, 0);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  dht.begin();
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  };

  pinMode(led, OUTPUT);
  Blynk.begin(auth, ssid, password);

  // Transmitter is connected to Arduino Pin #16  
  mySwitch.enableTransmit(16);
  mySwitch.setPulseLength(184);

  display.init();
  display.flipScreenVertically();
  display.setFont(Dialog_plain_16);
  digitalWrite(led, 1);

  timer.setInterval(2000, repeatMe);

}

void loop() {

  timer.run();
  Blynk.run();
  display.display();
  
}
