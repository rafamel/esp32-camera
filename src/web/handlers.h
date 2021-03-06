#include "http.h"

/* Types */
typedef struct {
  size_t len;
  uint8_t* buf;
  http_code_t status;
  const char* contentType;
} handler_response_t;

/* Agnostic */
void clear_handler_response(handler_response_t* response);
handler_response_t* handler_api_params();
handler_response_t* handler_api_status();
handler_response_t* handler_api_control(char* property, char* value);
