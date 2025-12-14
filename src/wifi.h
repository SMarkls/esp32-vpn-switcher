#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_interface.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include <esp_err.h>
#include <esp_event.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <string.h>
typedef enum {
  OK = 0,
  ERROR_DRIVER_INITIALIZING = 1,
  ERROR_STARTING = 2,
  ERROR_CONNECTING = 3,
} WIFI_ERROR;

void wifi_init(void);
