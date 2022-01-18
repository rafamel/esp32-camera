#include "Arduino.h"

#include "../../camera/camera.h"
#include "../../configuration.h"
#include "./constants.h"
#include "./handlers.h"

static handler_response_t* create_response(
  const char* status,
  const char* contentType,
  char* str
) {
  handler_response_t* response =
    (handler_response_t*) malloc(sizeof(handler_response_t));

  size_t len = 0;
  uint8_t* buf = NULL;

  if (str != NULL) {
    len = strlen(str);
    buf = (uint8_t*) malloc(len);
    memcpy(buf, str, len);
  }

  response->len = len;
  response->buf = buf;
  response->status = status;
  response->contentType = contentType;
  response->headerName = NULL;
  response->headerValue = NULL;

  return response;
}

void clear_handler_response(handler_response_t* response) {
  if (response->buf != NULL) free(response->buf);
  free(response);
}

handler_response_t* handler_redirect(const char* location) {
  handler_response_t* res = create_response(HTTP_302, NULL, NULL);
  res->headerName = "Location";
  res->headerValue = location;
  return res;
}

handler_response_t* handler_api_params() {
  static int port = SERVER_ASYNC ? SERVER_PORT : SERVER_PORT + 1;

  char p[1024];
  char* i = p;

  *i++ = '{';
  i += sprintf(i, " \"streamPort\": %d,", port);
  i += sprintf(i, " \"cameraModel\": \"%s\" ", "OV2640");
  *i++ = '}';
  *i++ = 0;

  handler_response_t* res = create_response(HTTP_200, "application/json", p);
  return res;
}

handler_response_t* handler_api_status() {
  char* status = get_json_camera_status();

  return status == NULL
    ? create_response(HTTP_500, "application/json", (char*) API_ERROR)
    : create_response(HTTP_200, "application/json", status);
}

handler_response_t* handler_api_control(char* property, char* value) {
  if (
    property == NULL
      || value == NULL
      || strlen(property) == 0
      || strlen(value) == 0
  ) {
    return create_response(HTTP_400, "application/json", (char*) API_ERROR);
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
    ? create_response(HTTP_200, "application/json", (char*) API_SUCCESS)
    : create_response(HTTP_500, "application/json", (char*) API_ERROR);
}
