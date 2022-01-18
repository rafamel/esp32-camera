#include "Arduino.h"
#include "esp_http_server.h"
#include "SPIFFS.h"

#include "../configuration.h"
#include "./handlers/handlers.h"
#include "./handlers/handlers_sync.h"

static esp_err_t execute_handler(httpd_req_t *req, handler_response_t* res) {
  esp_err_t err = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (err == ESP_OK) err = httpd_resp_set_status(req, res->status);
  if (err == ESP_OK && res->contentType != NULL) {
    err = httpd_resp_set_type(req, res->contentType);
  }
  if (err == ESP_OK && res->headerName != NULL && res->headerValue != NULL) {
    httpd_resp_set_hdr(req, res->headerName, res->headerValue);
  }
  if (err == ESP_OK) {
    err = httpd_resp_send(req, (const char*)res->buf, res->len);
  }

  clear_handler_response(res);
  return err;
}

esp_err_t start_server_sync() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  /* Web */
  static httpd_handle_t web_httpd = NULL;
  config.server_port = SERVER_PORT;
  config.ctrl_port = SERVER_PORT;

  if (httpd_start(&web_httpd, &config) != ESP_OK) {
    Serial.printf("Web server: fail");
    return ESP_FAIL;
  }

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return handler_file_sync(req, SPIFFS, "/index.html", "text/html");
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &index_uri);

  httpd_uri_t index_redirect_uri = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return execute_handler(req, handler_redirect("/"));
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &index_redirect_uri);

  httpd_uri_t styles_uri = {
    .uri       = "/styles.css",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return handler_file_sync(req, SPIFFS, "/styles.css", "text/css");
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &styles_uri);

  httpd_uri_t scripts_uri = {
    .uri       = "/scripts.js",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return handler_file_sync(req, SPIFFS, "/scripts.js", "text/javascript");
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &scripts_uri);

  httpd_uri_t api_params_uri = {
    .uri       = "/api/params",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return execute_handler(req, handler_api_params());
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &api_params_uri);

  httpd_uri_t api_status_uri = {
    .uri       = "/api/status",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      return execute_handler(req, handler_api_status());
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &api_status_uri);

  httpd_uri_t api_control_uri = {
    .uri       = "/api/control",
    .method    = HTTP_GET,
    .handler   = ([](httpd_req_t *req) {
      char* buf;
      size_t buf_len;
      char prop[32] = {0,};
      char value[32] = {0,};
      esp_err_t err = ESP_FAIL;

      buf_len = httpd_req_get_url_query_len(req) + 1;
      buf = (char*)malloc(buf_len);
      if (buf_len > 1 && buf) {
        err = httpd_req_get_url_query_str(req, buf, buf_len);
        if (err == ESP_OK) {
          err = httpd_query_key_value(buf, "property", prop, sizeof(prop));
        }
        if (err == ESP_OK) {
          err = httpd_query_key_value(buf, "value", value, sizeof(value));
        }
      }

      free(buf);
      return execute_handler(
        req,
        err == ESP_OK
          ? handler_api_control(prop, value)
          : handler_api_control(NULL, NULL)
      );
    }),
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &api_control_uri);

  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = handler_capture_sync,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(web_httpd, &capture_uri);

  Serial.printf("Web server: %d\n", config.server_port);

  /* Stream */
  static httpd_handle_t stream_httpd = NULL;
  config.server_port = SERVER_PORT + 1;
  config.ctrl_port = SERVER_PORT + 1;

  if (httpd_start(&stream_httpd, &config) != ESP_OK) {
    Serial.printf("Stream server: fail");
    return ESP_FAIL;
  }

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = handler_stream_sync,
    .user_ctx  = NULL
  };
  httpd_register_uri_handler(stream_httpd, &stream_uri);

  Serial.printf("Stream server: %d\n", config.server_port);
  return ESP_OK;
}
