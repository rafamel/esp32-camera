#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "../camera/camera.h"
#include "../configuration.h"
#include "./definitions.h"

void stream_handler(AsyncWebServerRequest *request);

void start_server() {
  // Create AsyncWebServer object
  static AsyncWebServer server(SERVER_PORT);

  /* Configure Headers */
  DefaultHeaders::Instance()
    .addHeader("Access-Control-Allow-Origin", "*");

  /* Catch-All Handlers */
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  // Static files
  server.serveStatic("/static", SPIFFS, "/static");

  /* Index */
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  /* Status */
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    char* status = get_json_camera_status();
    return status == NULL
      ? request->send(500, "application/json", JSON_ERROR)
      : request->send(200, "application/json", status);
  });

  /* Control */
  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasArg("property") || !request->hasArg("value")) {
      return request->send(400, "application/json", JSON_ERROR);
    }

    esp_err_t err = ESP_OK;
    int value = atoi(request->arg("value").c_str());
    const char* property = request->arg("property").c_str();

    if (!strcmp(property, "flashled")) {
      if (value > 0) digitalWrite(STATUS_LED_PIN, HIGH);
      else digitalWrite(STATUS_LED_PIN, LOW);
    } else {
      err = set_camera_status_property(property, value);
    }

    return err == ESP_OK
      ? request->send(200, "application/json", JSON_SUCCESS)
      : request->send(500, "application/json", JSON_ERROR);
  });

  /* Capture */
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    camera_capture_t* capture = get_camera_capture();
    if (capture == NULL) return request->send(500);

    request->onDisconnect([capture]() {
      release_camera_capture(capture);
    });

    AsyncWebServerResponse *response = request->beginResponse_P(
      200,
      "image/jpeg",
      capture->buf,
      capture->len
    );
    response->addHeader(
      "Content-Disposition",
      "inline; filename=capture.jpg"
    );
    request->send(response);
  });

  /* Stream */
  server.on("/stream", HTTP_GET, stream_handler);

  /* Start server */
  Serial.printf("Web server: %d\n", SERVER_PORT);
  server.begin();
}