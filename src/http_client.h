#include <stdbool.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>

bool is_interface_enabled(char *ifname);
void enable_interface(char *ifname);
