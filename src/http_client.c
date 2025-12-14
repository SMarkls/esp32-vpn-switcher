#include "http_client.h"

static const char *TAG = "INTERFACE_CONTROL";

#define OPENWRT_IP "192.168.1.1"
#define OPENWRT_PORT "80"
#define CGI_ENDPOINT "/cgi-bin/if-restart.lua"

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (!esp_http_client_is_chunked_response(evt->client)) {
      ESP_LOGI(TAG, "RESPONSE: %.*s", evt->data_len, (char *)evt->data);
    }
    break;
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP REQUEST ERROR");
    break;
  default:
    break;
  }
  return ESP_OK;
}

void enable_interface(char *ifname) {
  if (ifname == NULL || strlen(ifname) == 0) {
    ESP_LOGE(TAG, "INVALID INTERFACE NAME");
    return;
  }

  ESP_LOGI(TAG, "ENABLING INTERFACE: %s", ifname);
  char url[256];
  snprintf(url, sizeof(url), "http://%s:%s%s", OPENWRT_IP, OPENWRT_PORT,
           CGI_ENDPOINT);

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = http_event_handler,
      .disable_auto_redirect = false,
      .max_redirection_count = 3,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_header(client, "Content-Type", "text/plain");
  esp_http_client_set_header(client, "User-Agent", "ESP32-Interface-Control");
  esp_http_client_set_header(client, "Accept", "text/plain");
  esp_http_client_set_post_field(client, ifname, strlen(ifname));
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP STATUS CODE: %d", status_code);

    if (status_code == 200) {
      ESP_LOGI(TAG, "INTERFACE %s ENABLED SUCCESSFULLY", ifname);
    } else {
      ESP_LOGW(TAG, "INTERFACE %s ENABLE FAILED WITH STATUS: %d", ifname,
               status_code);
    }
  } else {
    ESP_LOGE(TAG, "HTTP REQUEST FAILED: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
}

static char interface_response_buffer[128];
static size_t interface_response_len = 0;

static esp_err_t interface_check_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (!esp_http_client_is_chunked_response(evt->client)) {
      size_t copy_len = evt->data_len;
      if (interface_response_len + copy_len >=
          sizeof(interface_response_buffer)) {
        copy_len =
            sizeof(interface_response_buffer) - interface_response_len - 1;
      }

      if (copy_len > 0) {
        memcpy(interface_response_buffer + interface_response_len, evt->data,
               copy_len);
        interface_response_len += copy_len;
        interface_response_buffer[interface_response_len] = '\0';
      }
    }
    break;

  case HTTP_EVENT_ON_FINISH:
    for (size_t i = 0; i < interface_response_len; i++) {
      interface_response_buffer[i] =
          tolower((unsigned char)interface_response_buffer[i]);
    }
    break;

  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP REQUEST ERROR IN INTERFACE CHECK");
    break;

  default:
    break;
  }
  return ESP_OK;
}

bool is_interface_enabled(char *ifname) {
  if (ifname == NULL || strlen(ifname) == 0) {
    ESP_LOGE(TAG, "INVALID INTERFACE NAME");
    return false;
  }

  ESP_LOGI(TAG, "CHECKING INTERFACE STATUS: %s", ifname);

  memset(interface_response_buffer, 0, sizeof(interface_response_buffer));
  interface_response_len = 0;

  char url[256];
  snprintf(url, sizeof(url), "http://%s:%s/cgi-bin/if-status.lua", OPENWRT_IP,
           OPENWRT_PORT);

  char request_body[64];
  snprintf(request_body, sizeof(request_body), "%s", ifname);

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = interface_check_event_handler,
      .timeout_ms = 8000,
      .disable_auto_redirect = false,
      .max_redirection_count = 2,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_header(client, "Content-Type",
                             "text/plain");
  esp_http_client_set_header(client, "User-Agent", "ESP32-Interface-Check");
  esp_http_client_set_header(client, "Accept", "text/plain");

  esp_http_client_set_post_field(client, request_body, strlen(request_body));

  esp_err_t err = esp_http_client_perform(client);
  bool interface_enabled = false;

  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP STATUS CODE: %d", status_code);

    if (status_code == 200 && interface_response_len > 0) {
      char *response = interface_response_buffer;
      while (*response == ' ' || *response == '\n' || *response == '\r' ||
             *response == '\t')
        response++;
      if (strstr(response, "true") != NULL) {
        interface_enabled = true;
        ESP_LOGI(TAG, "INTERFACE %s IS ENABLED", ifname);
      } else if (strstr(response, "false") != NULL) {
        interface_enabled = false;
        ESP_LOGI(TAG, "INTERFACE %s IS DISABLED", ifname);
      } else {
        if (strcmp(response, "1") == 0 || strcmp(response, "yes") == 0 ||
            strcmp(response, "enabled") == 0 || strcmp(response, "up") == 0) {
          interface_enabled = true;
        } else if (strcmp(response, "0") == 0 || strcmp(response, "no") == 0 ||
                   strcmp(response, "disabled") == 0 ||
                   strcmp(response, "down") == 0) {
          interface_enabled = false;
        } else {
          ESP_LOGW(TAG, "UNKNOWN RESPONSE FORMAT: %s", response);
          interface_enabled = false;
        }
      }
    } else {
      ESP_LOGW(TAG, "FAILED TO GET VALID RESPONSE FOR INTERFACE %s", ifname);
      interface_enabled = false;
    }
  } else {
    ESP_LOGE(TAG, "HTTP REQUEST FAILED: %s", esp_err_to_name(err));
    interface_enabled = false;
  }

  esp_http_client_cleanup(client);
  return interface_enabled;
}
