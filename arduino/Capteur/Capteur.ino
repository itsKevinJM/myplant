#include <DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
IPAddress server(192, 168, 60, 108);

WiFiClient WClient;
PubSubClient client(WClient);

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0; // Variable for storing previous time
const long interval1 = 10000;     // Interval for publishing data every 10 seconds
const long interval2 = 600000;    // Interval for publishing data every 10 minutes

void setup()
{
  Serial.begin(9600);
  dht.begin();

  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
    client.setServer(server, 1883);
    client.setCallback(callback);
  }

  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
}

void loop()
{
  unsigned long currentMillis = millis();

  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  // Check if 10 seconds have passed
  if (currentMillis - previousMillis >= interval1)
  {
    previousMillis = currentMillis;
    // Publish data to MQTT
    String data = String(t) + "," + String(h); // Format: "temperature,humidity"
    client.publish("data_pant", data.c_str());
  }

  // Check if 10 minutes have passed
  if (currentMillis - previousMillis >= interval2)
  {
    previousMillis = currentMillis;
    // Publish data to MQTT
    String data = String(t) + "," + String(h); // Format: "temperature,humidity"
    client.publish("data_pant", data.c_str());
  }

  // Maintain MQTT connection
  if (!client.connected())
    reconnect();
  client.loop();
}