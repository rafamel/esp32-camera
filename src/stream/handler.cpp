#include "Arduino.h"
#include "esp_http_server.h"

#include "../configuration.h"
#include "../camera/camera.h"
#include "../store/store.h"
#include "./constants.h"

esp_err_t handler_stream(httpd_req_t *req) {
  Serial.println("Camera stream: start");

  esp_err_t res = ESP_OK;
  camera_capture_t* cycle_capture = NULL;
  char * cycle_part[64];

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    Serial.println("Camera stream: http response type error");
    return res;
  }

  int64_t time_start;
  int64_t time_wait;
  int max_fps = 100;
  int time_per_frame = 0;
  while (true) {
    time_start = esp_timer_get_time();

    cycle_capture = get_camera_capture();
    if (cycle_capture == NULL) res = ESP_FAIL;

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(
        req,
        STREAM_BOUNDARY,
        strlen(STREAM_BOUNDARY)
      );
    }

    if (res == ESP_OK) {
      size_t hlen = snprintf(
        (char *)cycle_part,
        64,
        STREAM_PART,
        cycle_capture->len
      );
      res = httpd_resp_send_chunk(
        req, 
        (const char *)cycle_part,
        hlen
      );
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(
        req,
        (const char *)cycle_capture->buf,
        cycle_capture->len
      );
    }

    if (cycle_capture != NULL) {
      release_camera_capture(cycle_capture);
      cycle_capture = NULL;
    }

    if (res != ESP_OK) break;

    max_fps = store_get_stream_fps();
    time_per_frame = 1000000 / max_fps;
    time_wait = time_per_frame - (esp_timer_get_time() - time_start);
    if (time_wait > 0) delay(time_wait / 1000);
  }

  Serial.println("Camera stream: stop");
  return res;
}
