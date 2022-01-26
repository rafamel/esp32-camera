#include "Arduino.h"
#include "http.h"

#include "../camera/camera.h"
#include "../configuration.h"
#include "./handlers.h"

static const char* api_error = "{ \"error\": true }";
static const char* api_success = "{ \"success\": true }";

static handler_response_t* create_response(
  http_code_t status,
  const char* contentType,
  char* str
) {
  handler_response_t* response =
    (handler_response_t*) malloc(sizeof(handler_response_t));

  size_t len = 0;
  uint8_t* buf = NULL;

  if (str == api_error || str == api_success) {
    len = strlen(str);
    buf = (uint8_t*) str;
  } else if (str != NULL) {
    len = strlen(str);
    buf = (uint8_t*) malloc(len);
    memcpy(buf, str, len);
  }

  response->len = len;
  response->buf = buf;
  response->status = status;
  response->contentType = contentType;

  return response;
}

void clear_handler_response(handler_response_t* response) {
  bool free_buf = response->buf != NULL
    && response->buf != (uint8_t*) api_error
    && response->buf != (uint8_t*) api_success;

  if (free_buf) free(response->buf);
  free(response);
}

handler_response_t* handler_api_params() {
  StaticJsonDocument<512> doc;

  doc["streamPort"] = STREAM_SERVER_PORT;
  doc["cameraModel"] = get_camera_model();

  String json;
  return serializeJsonPretty(doc, json)
    ? create_response(HTTP_200, "application/json", (char*) json.c_str())
    : create_response(HTTP_500, "application/json", (char*) api_error);
}

handler_response_t* handler_api_status() {
  StaticJsonDocument<1024> doc;

  get_camera_status(&doc);
  doc["flashled"] = digitalRead(STATUS_LED_PIN) == HIGH ? 1 : 0;

  String json;
  return serializeJsonPretty(doc, json)
    ? create_response(HTTP_200, "application/json", (char*) json.c_str())
    : create_response(HTTP_500, "application/json", (char*) api_error);
}

handler_response_t* handler_api_control(char* property, char* value) {
  if (
    property == NULL
      || value == NULL
      || strlen(property) == 0
      || strlen(value) == 0
  ) {
    return create_response(HTTP_400, "application/json", (char*) api_error);
  }

  int digit = atoi(value);
  bool success = true;

  if (!strcmp(property, "flashled")) {
    if (digit > 0) digitalWrite(STATUS_LED_PIN, HIGH);
    else digitalWrite(STATUS_LED_PIN, LOW);
  } else {
    success = set_camera_status_property(property, digit) == ESP_OK;
  }

  return success
    ? create_response(HTTP_200, "application/json", (char*) api_success)
    : create_response(HTTP_500, "application/json", (char*) api_error);
}
