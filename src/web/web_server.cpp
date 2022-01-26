#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "result.h"

#include "../camera/camera.h"
#include "./handlers.h"

static void execute_handler(
  AsyncWebServerRequest *req,
  handler_response_t* res
) {
  req->onDisconnect([res]() {
    clear_handler_response(res);
  });

  req->send_P(atoi(res->status), res->contentType, res->buf, res->len);
}

result_t start_web_server(uint8_t server_port) {
  // Create AsyncWebServer object
  static AsyncWebServer server(server_port);

  /* Configure Headers */
  DefaultHeaders::Instance()
    .addHeader("Access-Control-Allow-Origin", "*");

  /* Catch-All Handlers */
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  /* Assets */
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  /* Params */
  server.on("/api/params", HTTP_GET, [](AsyncWebServerRequest *request) {
    execute_handler(request, handler_api_params());
  });

  /* Status */
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    execute_handler(request, handler_api_status());
  });

  /* Control */
  server.on("/api/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    execute_handler(
      request,
      handler_api_control(
        (char*) request->arg("property").c_str(),
        (char*) request->arg("value").c_str()
      )
    );
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

  /* Start server */
  Serial.printf("Web server: %d\n", server_port);
  server.begin();

  return RESULT_OK;
}
