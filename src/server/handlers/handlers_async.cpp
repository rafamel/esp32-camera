#include "Arduino.h"
#include "ESPAsyncWebServer.h"

#include "../../configuration.h"
#include "../../camera/camera.h"
#include "./constants.h"

void handler_capture_async(AsyncWebServerRequest *request) {
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
}

void handler_stream_async(AsyncWebServerRequest *request) {
  Serial.println("Camera stream: start");

  // Allocate memory to pointers
  size_t* data_i = (size_t*) malloc(sizeof(size_t));
  size_t* data_len = (size_t*) malloc(sizeof(size_t));
  int* stage_i = (int*) malloc(sizeof(int));
  int* cycle_i = (int*) malloc(sizeof(int));
  char** cycle_part = (char**) malloc(sizeof(char*));
  camera_capture_t** cycle_capture = (camera_capture_t**) malloc(sizeof(camera_capture_t*));
  size_t* cycle_max_len = (size_t*) malloc(sizeof(size_t));

  // Initialize variables
  *data_i = 0;
  *data_len = 0;
  *stage_i = 0;
  *cycle_i = 0;
  *cycle_part = NULL;
  *cycle_capture = NULL;
  *cycle_max_len = ESP.getFreeHeap() / 3;

  // Teardown
  request->onDisconnect([data_i, data_len, stage_i, cycle_i, cycle_part, cycle_capture, cycle_max_len]() {
    Serial.println("Camera stream: stop");
    if (*cycle_part != NULL) free(*cycle_part);
    if (*cycle_capture != NULL) release_camera_capture(*cycle_capture);
    free(data_i);
    free(data_len);
    free(stage_i);
    free(cycle_i);
    free(cycle_part);
    free(cycle_capture);
    free(cycle_max_len);
  });

  AsyncWebServerResponse *response = request->beginChunkedResponse(
    STREAM_CONTENT_TYPE,
    [data_i, data_len, stage_i, cycle_i, cycle_part, cycle_capture, cycle_max_len](
      uint8_t *it_buf, size_t it_max_len, size_t it_index
    ) -> size_t {
      size_t it_len = 0;
      size_t max_len = it_max_len;
      if (max_len > *cycle_max_len) max_len = *cycle_max_len;

      switch (*stage_i) {
        case 0: {
          if (*cycle_capture == NULL) {
            *data_len = strlen(STREAM_BOUNDARY);
            *cycle_capture = get_camera_capture();
            if (cycle_capture == NULL) return 0;
          }

          it_len = *data_len - *data_i;
          if (it_len > max_len) it_len = max_len;

          memcpy(it_buf, STREAM_BOUNDARY + *data_i, it_len);
          break;
        }
        case 1: {
          if (*cycle_part == NULL) {
            *cycle_part = (char*) malloc((sizeof(char) * 64) + 1);
            *data_len = snprintf(
              (char *)(*cycle_part),
              64,
              STREAM_PART,
              (*cycle_capture)->len
            );
          }

          it_len = *data_len - *data_i;
          if (it_len > max_len) it_len = max_len;

          memcpy(it_buf, *cycle_part + *data_i, it_len);
          break;
        }
        case 2: {
          if (*data_len <= 0) {
            *data_len = (*cycle_capture)->len;
          }

          it_len = *data_len - *data_i;
          if (it_len > max_len) it_len = max_len;

          memcpy(it_buf, (*cycle_capture)->buf + *data_i, it_len);
          break;
        }
        default: {
          it_len = 0;
        }
      }

      // Update current data index
      *data_i = *data_i + it_len;

      // Update stage variables on stage finalization
      if (*data_i >= *data_len) {
        *stage_i += 1;
        *data_i = 0;
        *data_len = 0;
        // Update cycle variables on cycle finalization
        if (*stage_i > 2) {
          free(*cycle_part);
          release_camera_capture(*cycle_capture);
          *stage_i = 0;
          *cycle_i += 1;
          *cycle_part = NULL;
          *cycle_capture = NULL;
          *cycle_max_len = ESP.getFreeHeap() / 3;
        }
      }

      return it_len;
    }
  );
  request->send(response);
}