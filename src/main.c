#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "wifi.h"
#include <stdlib.h>
#include "http_client.h"

#define BUTTON_PIN 27 // пин кнопки (S)
#define LED_PIN 2     // светодиод

void app_main(void) {
  gpio_reset_pin(LED_PIN);
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

  gpio_config_t btn_conf = {.pin_bit_mask = (1ULL << BUTTON_PIN),
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_ENABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&btn_conf);

  int count = 0;
  int ledValue = 0;
  int lasLevel = 1;
  wifi_init();

  gpio_set_level(LED_PIN, ledValue);
  while (1) {
    int level = gpio_get_level(BUTTON_PIN);
    if (count % 1000 == 0) {
      ESP_LOGI("BUTTON", "Button level %d", level);
      count = 0;
    }

    if (level == 0 && lasLevel == 1) {
      ESP_LOGW("BUTTON", "Button changed. %d", level);
      ledValue = !ledValue;
      gpio_set_level(LED_PIN, ledValue);
      if(is_interface_enabled("wg0")) {
        enable_interface("wan");
      }
      else {
        enable_interface("wg0");
      }
    }

    lasLevel = level;
    vTaskDelay(pdMS_TO_TICKS(50));
    count++;
  }
}
