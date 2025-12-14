#include "wifi.h"
#include <string.h>

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
#define ESP_NETIF_SUPPORTED
#endif

#ifdef ESP_NETIF_SUPPORTED
#include <esp_netif.h>
#else
#include <tcpip_adapter.h>
#endif
#define STR(x) #x
#define XSTR(x) STR(x)

const char *ssid = XSTR(WIFI_SSID);
const char *password = XSTR(WIFI_PASSWORD);

static bool isInitialized = false;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI("WIFI", "CONNECTED WITH IP ADDRESS:", IPSTR,
             IP2STR(&event->ip_info.ip));
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGI("WIFI", "DISCONNECTED. CONNECTING TO THE AP AGAIN..");
    esp_wifi_connect();
  }
}

void init_nvs() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
}

void wifi_init(void) {
  if (isInitialized) {
    ESP_LOGW("WIFI", "ALREADY CONNECTED");
    return;
  }
  init_nvs();
  /* Initialize TCP/IP */
#ifdef ESP_NETIF_SUPPORTED
  esp_netif_init();
#else
  tcpip_adapter_init();
#endif

  /* Initialize the event loop */
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* Register our event handler for Wi-Fi and IP related events */
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &event_handler, NULL));

#ifdef ESP_NETIF_SUPPORTED
  esp_netif_create_default_wifi_sta();
#endif

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);

  wifi_config_t wifi_config = {
      .sta = {.ssid = "", .password = ""},
  };

  strcpy((char *)wifi_config.sta.ssid, ssid);
  strcpy((char *)wifi_config.sta.password, password);

  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();
  isInitialized = true;
}
