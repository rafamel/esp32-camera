#include "Arduino.h"
#include "ArduinoJson.h"
#include "esp_camera.h"
#include "result.h"

#include "../configuration.h"
#include "camera_pins.h"
#include "camera.h"

static camera_fb_t* fb = NULL;
static camera_capture_t* capture_fb = NULL;

result_t start_camera() {
  /* Configure camera */
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // PSRAM IC present: init with UXGA resolution and higher
  // JPEG quality for larger pre-allocated frame buffer
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  /* Initialize camera */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) return RESULT_FAIL;

  /* Mofify camera defaults */
  sensor_t* s = esp_camera_sensor_get();
  if (s == NULL) return RESULT_FAIL;

  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

  // initial sensors are flipped vertically, colors saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }

  #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  #endif

  Serial.println("Camera: start");
  return RESULT_OK;
}

const char* get_camera_model() {
  sensor_t* s = esp_camera_sensor_get();

  return (s->id.PID == OV3660_PID) ? "OV3660" : "OV2640";
}

result_t get_camera_status(JsonDocument* doc) {
  sensor_t* s = esp_camera_sensor_get();
  if (s == NULL) return RESULT_FAIL;

  (*doc)["framesize"] = s->status.framesize;
  (*doc)["quality"] = s->status.quality;
  (*doc)["brightness"] = s->status.brightness;
  (*doc)["contrast"] = s->status.contrast;
  (*doc)["saturation"] = s->status.saturation;
  (*doc)["sharpness"] = s->status.sharpness;
  (*doc)["special_effect"] = s->status.special_effect;
  (*doc)["wb_mode"] = s->status.wb_mode;
  (*doc)["awb"] = s->status.awb;
  (*doc)["awb_gain"] = s->status.awb_gain;
  (*doc)["aec"] = s->status.aec;
  (*doc)["aec2"] = s->status.aec2;
  (*doc)["ae_level"] = s->status.ae_level;
  (*doc)["aec_value"] = s->status.aec_value;
  (*doc)["agc"] = s->status.agc;
  (*doc)["agc_gain"] =  s->status.agc_gain;
  (*doc)["gainceiling"] = s->status.gainceiling;
  (*doc)["bpc"] = s->status.bpc;
  (*doc)["wpc"] = s->status.wpc;
  (*doc)["raw_gma"] = s->status.raw_gma;
  (*doc)["lenc"] = s->status.lenc;
  (*doc)["vflip"] = s->status.vflip;
  (*doc)["hmirror"] = s->status.hmirror;
  (*doc)["dcw"] = s->status.dcw;
  (*doc)["colorbar"] = s->status.colorbar;

  return RESULT_OK;
}

result_t set_camera_status_property(const char* property, int value) {
  int res = 0;
  sensor_t* s = esp_camera_sensor_get();
  
  if (s == NULL) return RESULT_FAIL;

  if (!strcmp(property, "framesize")) {
    if (s->pixformat == PIXFORMAT_JPEG) {
      res = s->set_framesize(s, (framesize_t)value);
    }
  } else if (!strcmp(property, "quality")) {
    res = s->set_quality(s, value);
  } else if (!strcmp(property, "contrast")) {
    res = s->set_contrast(s, value);
  } else if (!strcmp(property, "brightness")) {
    res = s->set_brightness(s, value);
  } else if (!strcmp(property, "saturation")) {
    res = s->set_saturation(s, value);
  } else if (!strcmp(property, "gainceiling")) {
    res = s->set_gainceiling(s, (gainceiling_t)value);
  } else if (!strcmp(property, "colorbar")) {
    res = s->set_colorbar(s, value);
  } else if (!strcmp(property, "awb")) {
    res = s->set_whitebal(s, value);
  } else if (!strcmp(property, "agc")) {
    res = s->set_gain_ctrl(s, value);
  } else if (!strcmp(property, "aec")) {
    res = s->set_exposure_ctrl(s, value);
  } else if (!strcmp(property, "hmirror")) {
    res = s->set_hmirror(s, value);
  } else if (!strcmp(property, "vflip")) {
    res = s->set_vflip(s, value);
  } else if (!strcmp(property, "awb_gain")) {
    res = s->set_awb_gain(s, value);
  } else if (!strcmp(property, "agc_gain")) {
    res = s->set_agc_gain(s, value);
  } else if (!strcmp(property, "aec_value")) {
    res = s->set_aec_value(s, value);
  } else if (!strcmp(property, "aec2")) {
    res = s->set_aec2(s, value);
  } else if (!strcmp(property, "dcw")) {
    res = s->set_dcw(s, value);
  } else if (!strcmp(property, "bpc")) {
    res = s->set_bpc(s, value);
  } else if (!strcmp(property, "wpc")) {
    res = s->set_wpc(s, value);
  } else if (!strcmp(property, "raw_gma")) {
    res = s->set_raw_gma(s, value);
  } else if (!strcmp(property, "lenc")) {
    res = s->set_lenc(s, value);
  } else if (!strcmp(property, "special_effect")) {
    res = s->set_special_effect(s, value);
  } else if (!strcmp(property, "wb_mode")) {
    res = s->set_wb_mode(s, value);
  } else if (!strcmp(property, "ae_level")) {
    res = s->set_ae_level(s, value);
  } else if (!strcmp(property, "flashled")) {
    if (value > 0) digitalWrite(STATUS_LED_PIN, HIGH);
    else digitalWrite(STATUS_LED_PIN, LOW);
  } else {
    return RESULT_FAIL;
  }

  return res ? RESULT_FAIL : RESULT_OK;
}

camera_capture_t* get_camera_capture() {
  static int64_t last_frame = 0;

  int64_t time_start = esp_timer_get_time();
  float fps = last_frame > 0
    ? (1000.0 / ((time_start - last_frame) / 1000.0))
    : 0.0;
  bool fps_limit = fps > CAMERA_MAX_FPS;

  if (fb != NULL || fps_limit) {
    capture_fb->subs += 1;

    Serial.println("Camera capture: cache");
    return capture_fb;
  }

  fb = esp_camera_fb_get();

  if (fb == NULL) {
    Serial.println("Camera capture: frame request error");
    return get_camera_capture();
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Camera capture: image format error");
    return NULL;
  }

  camera_capture_t* capture = 
    (camera_capture_t*) malloc(sizeof(camera_capture_t));
  size_t capture_len = fb->len;
  uint8_t* capture_buf = (uint8_t*) malloc(capture_len);
  memcpy(capture_buf, fb->buf, capture_len);
  *capture = { capture_buf, capture_len, 1 };

  if (capture_fb != NULL && capture_fb->subs <= 0) {
    free(capture_fb->buf);
    free(capture_fb);
  }

  capture_fb = capture;
  last_frame = time_start;

  int64_t time_end = esp_timer_get_time();
  Serial.printf(
    "Camera capture: %uB %ums (%.1ffps)\r\n",
    (uint32_t)(fb->len),
    (uint32_t)((time_end - time_start) / 1000),
    fps
  );

  return capture;
}

bool release_camera_capture(camera_capture_t* capture) {
  if (capture == NULL) return false;

  capture->subs -= 1;

  if (capture == capture_fb) {
    if (fb != NULL) {
      esp_camera_fb_return(fb);
      fb = NULL;
    }
    return true;
  }

  if (capture->subs <= 0) {
    free(capture->buf);
    free(capture);
    return true;
  }

  return false;
}