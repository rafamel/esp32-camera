#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include "Arduino.h"

#include "camera/camera.h"
#include "configuration.h"

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  /* Initialize SPIFFS */
  if (SPIFFS.begin(true)) {
    Serial.println("FS: start");
  } else {
    Serial.println("FS: an error has occurred while mounting SPIFFS");
    ESP_ERROR_CHECK(ESP_FAIL);
  }

  /* WiFi Connection */
  #if ENABLE_WIFI_CONNECTION
    WiFi.begin(WIFI_CONNECTION_SSID, WIFI_CONNECTION_PASSWORD);
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_CONNECTION_SSID);
  #endif

  /* Access Point */
  #if ENABLE_ACCESS_POINT
    WiFi.softAP(ACCESS_POINT_SSID);
    IPAddress AP_IP = WiFi.softAPIP();
    Serial.print("Access Point SSID: ");
    Serial.println(ACCESS_POINT_SSID);
    Serial.print("Access Point IP: ");
    Serial.println(AP_IP);
  #endif

  /* Initialize MDNS */
  if (MDNS.begin(HOST_NAME)) {
    Serial.print("MDNS Host: ");
    Serial.println(HOST_NAME);
  } else {
    Serial.println("Error setting up MDNS");
  }

  /* Start camera */
  ESP_ERROR_CHECK(start_camera());
}

void loop() {
  delay(10000);
}
