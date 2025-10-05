/**
 * This example takes a picture every 5s and print its size on serial monitor.
 */

// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define BOARD_ESP32CAM_AITHINKER
// #define BOARD_ESP32S3_WROOM

/**
 * 2. Kconfig setup
 *
 * If you have a Kconfig file, copy the content from
 *  https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
 * In case you haven't, copy and paste this Kconfig file inside the src directory.
 * This Kconfig file has definitions that allows more control over the camera and
 * how it will be initialized.
 */

/**
 * 3. Enable PSRAM on sdkconfig:
 *
 * CONFIG_ESP32_SPIRAM_SUPPORT=y
 *
 * More info on
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
 */

// ================================ CODE ======================================

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"
#include "take_picture.h"


static const char *TAG = "mu_camera";

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // Use the defines from the header file for key parameters
    .xclk_freq_hz = CAM_XCLK_FREQ_HZ,
    .ledc_timer = CAM_LEDC_TIMER,
    .ledc_channel = CAM_LEDC_CHANNEL,

    .pixel_format = CAM_PIXEL_FORMAT,
    .frame_size = CAM_FRAME_SIZE,

    .jpeg_quality = CAM_JPEG_QUALITY,
    .fb_count = CAM_FB_COUNT,
    .fb_location = CAM_FB_LOCATION,
    .grab_mode = CAM_GRAB_MODE
};

esp_err_t init_camera(void)
{
    ESP_LOGI(TAG, "Initializing camera...");
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

camera_fb_t* take_picture(void)
{
    ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();
    if (!pic) {
        ESP_LOGE(TAG, "Failed to capture image");
        return NULL;
    }
    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    return pic;
}

// void app_main(void)
// {
//     if(ESP_OK != init_camera()) {
//         return;
//     }
//     init_uart();
//     while (1)
//     {
//         camera_fb_t *pic = take_picture();
//         if (pic) {
//             uart_write_bytes(UART_NUM_0, (const char *)pic->buf, pic->len);
//             esp_camera_fb_return(pic);
//         }
//         vTaskDelay(5000 / portTICK_RATE_MS);
//     }
// }
