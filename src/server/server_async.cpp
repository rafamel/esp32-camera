#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "result.h"

#include "../configuration.h"
#include "./handlers/handlers.h"
#include "./handlers/handlers_async.h"

static void execute_handler(
  AsyncWebServerRequest *req,
  handler_response_t* res
) {
  req->onDisconnect([res]() {
    clear_handler_response(res);
  });

  AsyncWebServerResponse *response = req->beginResponse_P(
    atoi(res->status),
    res->contentType,
    res->buf,
    res->len
  );

  if (res->headerName != NULL && res->headerValue != NULL) {
    response->addHeader(res->headerName, res->headerValue);
  }

  req->send(response);
}

result_t start_server_async() {
  // Create AsyncWebServer object
  static AsyncWebServer server(SERVER_PORT);

  /* Configure Headers */
  DefaultHeaders::Instance()
    .addHeader("Access-Control-Allow-Origin", "*");

  /* Catch-All Handlers */
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  /* Assets */
  server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest *request) {
    execute_handler(request, handler_redirect("/"));
  });

  server.on("/styles.css", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/styles.css", "text/css");
  });

  server.on("/scripts.js", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/scripts.js", "text/javascript");
  });

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
  server.on("/capture", HTTP_GET, handler_capture_async);

  /* Stream */
  server.on("/stream", HTTP_GET, handler_stream_async);

  /* Start server */
  Serial.printf("Web server: %d\n", SERVER_PORT);
  server.begin();

  return RESULT_OK;
}
