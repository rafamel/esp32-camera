#include <sys/stat.h>
#include "Arduino.h"
#include "esp_http_server.h"
#include "FS.h"
#include "http.h"

#include "../../configuration.h"
#include "../../camera/camera.h"
#include "./constants.h"

esp_err_t handler_file_sync(
  httpd_req_t *req,
  FS &fs,
  const char* filepath,
  const char* contentType
) {
  esp_err_t err = ESP_OK;
  esp_err_t ferr = ESP_OK;

  err = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (err != ESP_OK) return err;

  File file = fs.open(filepath, FILE_READ);
  if (!file) {
    Serial.printf("File handler: file not found \"%s\"\n", filepath);

    err = httpd_resp_set_status(req, HTTP_404);
    return (err == ESP_OK) ? httpd_resp_send(req, NULL, 0) : err;
  }

  if (file.isDirectory()) {
    Serial.println("File handler: path must not be a directory");

    file.close();
    err = httpd_resp_set_status(req, HTTP_500);
    return (err == ESP_OK) ? httpd_resp_send(req, NULL, 0) : err;
  }

  err = httpd_resp_set_type(req, contentType);
  if (err != ESP_OK) return err;

  Serial.printf(
    "File handler: send file \"%s\" (%d bytes)\n",
    filepath,
    file.size()
  );

  size_t file_len = file.size();
  size_t max_len = ESP.getFreeHeap() / 4;
  if (max_len > file_len) max_len = file_len;

  size_t chunk_i = 0;
  size_t chunk_len;
  uint8_t* chunk_buf = (uint8_t*) malloc(max_len);

  while (chunk_i < file_len) {
    chunk_len = file_len - chunk_i;
    if (chunk_len > max_len) chunk_len = max_len;

    if (file.read(chunk_buf, chunk_len)) ferr = ESP_FAIL;

    ferr = httpd_resp_send_chunk(req, (char*) chunk_buf, chunk_len);
    if (ferr != ESP_OK) break;

    chunk_i += chunk_len;
  }

  free(chunk_buf);
  file.close();

  err = httpd_resp_set_status(req, ferr == ESP_OK ? HTTP_200 : HTTP_500);
  if (err == ESP_OK) err = httpd_resp_send_chunk(req, NULL, 0);

  return ferr == ESP_OK ? err : ferr;
}

esp_err_t handler_capture_sync(httpd_req_t *req) {
  esp_err_t err = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (err != ESP_OK) return err;

  camera_capture_t* capture = get_camera_capture();

  if (capture == NULL) {
    if (err == ESP_OK) err = httpd_resp_set_status(req, HTTP_500);
    return httpd_resp_send(req, NULL, 0);
  }

  if (err == ESP_OK) err = httpd_resp_set_status(req, HTTP_200);
  if (err == ESP_OK) err = httpd_resp_set_type(req, "image/jpeg");
  if (err == ESP_OK) {
    err = httpd_resp_set_hdr(
      req,
      "Content-Disposition",
      "inline; filename=capture.jpg"
    );
  }
  if (err == ESP_OK) {
    err = httpd_resp_send(req, (const char*)capture->buf, capture->len);
  }

  release_camera_capture(capture);
  return err;
}

esp_err_t handler_stream_sync(httpd_req_t *req) {
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

  while (true) {
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
  }

  Serial.println("Camera stream: stop");
  return res;
}

