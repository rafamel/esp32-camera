#include "ArduinoJson.h"
#include "esp_camera.h"
#include "result.h"

typedef struct {
  uint8_t* buf;
  size_t len;
  int subs;
} camera_capture_t;

result_t start_camera();
const char* get_camera_model();
result_t get_camera_status(JsonDocument* doc);
result_t set_camera_status_property(const char* property, int value);
camera_capture_t* get_camera_capture();
bool release_camera_capture(camera_capture_t* capture);
