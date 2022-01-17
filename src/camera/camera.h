#include "esp_camera.h"

typedef struct {
  uint8_t *buf;
  size_t len;
} camera_capture_t;

esp_err_t start_camera();
const char* get_camera_model();
char* get_json_camera_status();
esp_err_t set_camera_status_property(const char* property, int value);
camera_capture_t* get_camera_capture();
bool release_camera_capture(camera_capture_t* capture);
