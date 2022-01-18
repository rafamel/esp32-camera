#include "esp_http_server.h"

esp_err_t handler_file_sync(
  httpd_req_t *req,
  FS &fs,
  const char* filepath,
  const char* contentType
);
esp_err_t handler_capture_sync(httpd_req_t *req);
esp_err_t handler_stream_sync(httpd_req_t *req);
