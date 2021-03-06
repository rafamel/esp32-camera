/* Camera Model */
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

/* Status LED */
#define STATUS_LED_PIN 4

/* Services */
#define HOST_NAME "camera"

/* Web Server */
#define WEB_SERVER_PORT 80

/* Stream Server */
#define STREAM_SERVER_PORT WEB_SERVER_PORT + 1
#define STREAM_DEFAULT_FPS 15

/* Access Point */
#define ENABLE_ACCESS_POINT true
#define ACCESS_POINT_SSID "Camera"

/* WiFi Connection */
#define ENABLE_WIFI_CONNECTION false
#define WIFI_CONNECTION_SSID ""
#define WIFI_CONNECTION_PASSWORD ""
