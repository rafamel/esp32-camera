#include "Arduino.h"
#include "esp_http_server.h"
#include "result.h"

#include "../configuration.h"

esp_err_t handler_stream(httpd_req_t *req);

result_t start_stream_server(uint8_t server_port) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  static httpd_handle_t stream_httpd = NULL;
  config.server_port = server_port;
  config.ctrl_port = server_port;
  config.max_uri_handlers = 8;

  if (httpd_start(&stream_httpd, &config) != ESP_OK) {
    Serial.printf("Stream server: fail");
    return RESULT_FAIL;
  }

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = handler_stream,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(stream_httpd, &stream_uri);

  Serial.printf("Stream server: %d\n", server_port);
  return RESULT_OK;
}
