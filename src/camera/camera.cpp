#include "Arduino.h"
#include "esp_camera.h"
#include "result.h"

#include "../configuration.h"
#include "camera_pins.h"
#include "camera.h"

static camera_fb_t* fb = NULL;
static bool keep_fb = false;
static camera_capture_t capture_fb;
static camera_capture_t capture_a;
static camera_capture_t capture_b;
static int capture_a_subs = 0;
static int capture_b_subs = 0;

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

  if (s->id.PID == OV3660_PID) {
    return "OV3660";
  }
  return "OV2640";
}

char* get_json_camera_status() {
  static char json_response[1024];

  sensor_t* s = esp_camera_sensor_get();
  if (s == NULL) return NULL;

  char* p = json_response;
  *p++ = '{';

  p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p+=sprintf(p, "\"quality\":%u,", s->status.quality);
  p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
  p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p+=sprintf(p, "\"awb\":%u,", s->status.awb);
  p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p+=sprintf(p, "\"aec\":%u,", s->status.aec);
  p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p+=sprintf(p, "\"agc\":%u,", s->status.agc);
  p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
  p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
  *p++ = '}';
  *p++ = 0;

  return json_response;
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

  if (keep_fb) {
    Serial.println("Camera capture: cache");
    if (capture_a_subs > 0 && capture_b_subs <= 0) {
      capture_b = capture_a;
      capture_b_subs = capture_a_subs;
      capture_a_subs = 0;
    }

    if (capture_a_subs <= 0) {
      uint8_t* buf = (uint8_t*) malloc(capture_fb.len);
      memcpy(buf, capture_fb.buf, capture_fb.len);
      capture_a = { buf, capture_fb.len };
    }
    
    capture_a_subs += 1;
    return &capture_a;
  }
  
  int64_t time_start = esp_timer_get_time();
  if (
    last_frame > 0 && 
    (time_start - last_frame) < (1000000.0 / CAMERA_MAX_FPS)
  ) {
    keep_fb = true;
    return &capture_fb;
  }
  
  if (fb != NULL) esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();

  if (fb == NULL) {
    Serial.println("Camera capture: frame request error");
    return NULL;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Camera capture: image format error");
    return NULL;
  }

  int64_t time_end = esp_timer_get_time();
  int64_t frame_time = last_frame > 0 
    ? (time_start - last_frame) / 1000
    : 100000;

  Serial.printf(
    "Camera capture: %uB %ums (%.1ffps)\r\n",
    (uint32_t)(fb->len),
    (uint32_t)((time_end - time_start) / 1000),
    1000.0 / (uint32_t)frame_time
  );

  keep_fb = true;
  last_frame = time_start;
  capture_fb = { fb->buf, fb->len };
  return &capture_fb;
}

bool release_camera_capture(camera_capture_t* capture) {
  if (keep_fb && capture == &capture_fb) {
    keep_fb = false;
    return true;
  }

  if (capture_b_subs > 0 && capture == &capture_b) {
    capture_b_subs -= 1;
    if (capture_b_subs == 0) free(capture_b.buf);
    return true;
  }

  if (capture_a_subs > 0 && capture == &capture_a) {
    capture_a_subs -= 1;
    if (capture_a_subs == 0) free(capture_a.buf);
    return true;
  }

  return false;
}