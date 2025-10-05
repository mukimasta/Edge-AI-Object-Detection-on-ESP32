#ifndef MU_WIFI_H
#define MU_WIFI_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Initialize the WiFi module in STA mode
 * 
 * This function handles the low-level initialization of the WiFi stack.
 * It's called internally by connect_to_wifi, but can be called separately
 * if custom initialization is needed.
 * 
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t init_wifi(void);

/**
 * @brief Connect to WiFi with the given SSID and password
 * 
 * This function initializes the WiFi module and attempts to connect
 * to the specified access point.
 * 
 * @param ssid WiFi SSID to connect to
 * @param password Password for the WiFi network
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t connect_to_wifi(const char* ssid, const char* password);



#ifdef __cplusplus
}
#endif

#endif /* MU_WIFI_H */
