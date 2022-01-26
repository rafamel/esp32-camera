#include "Arduino.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include "result.h"

#include "camera/camera.h"
#include "configuration.h"

result_t start_web_server(uint8_t server_port);
result_t start_stream_server(uint8_t server_port);

void setup() {
  result_t res = RESULT_OK;

  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  /* Initialize SPIFFS */
  if (SPIFFS.begin(true)) {
    Serial.println("FS: start");
  } else {
    res = RESULT_FAIL;
    Serial.println("FS: an error has occurred while mounting SPIFFS");
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
    res = RESULT_FAIL;
    Serial.println("Error setting up MDNS");
  }

  /* Setup Status LED */
  pinMode(STATUS_LED_PIN, OUTPUT);

  /* Flash LED x5 */
  int i;
  for (i = 1; i <= 5; i += 1) {
    delay(500);
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
  }

  /* Start camera */
  if (start_camera() != RESULT_OK) res = RESULT_FAIL;

  /* Web Server */
  if (start_web_server(WEB_SERVER_PORT) != RESULT_OK) res = RESULT_FAIL;

  /* Stream Server */
  if (start_stream_server(STREAM_SERVER_PORT) != RESULT_OK) res = RESULT_FAIL;

  if (res != RESULT_OK) exit(EXIT_FAILURE);
}

void loop() {
  delay(10000);
}
