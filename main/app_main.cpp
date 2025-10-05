#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#include <string>

#include "mu_detector.hpp"
#include "mu_wifi.h"
#include "mu_server.hpp"
#include "take_picture.h"


const char *TAG = "MAIN";
// #define WIFI_SSID "Mukii"
// #define WIFI_PASSWORD "mukimasta"
#define WIFI_SSID "NUSRI-STU"
#define WIFI_PASSWORD "Nusri311"

mu::MuDetector my_detector;
mu::MuServer my_server;

esp_err_t app_init(void);

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting application...");
    esp_err_t app_init_err = 
        app_init();
    if (app_init_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize application");
        my_server.display_system_message("ERROR: Application initialization failed");
        return;
    }

    ESP_LOGI(TAG, "Successfully initialized system");
    my_server.display_system_message("System initialized successfully");
    ESP_LOGI(TAG, "Starting main loop...");
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // test message
    // while (1) { 
    //     ESP_LOGI(TAG, "Test message");
    //     my_server.display_system_message("Test message");
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }

    while (1) {
        // Take a picture
        camera_fb_t *pic = take_picture();
        if (!pic) {
            ESP_LOGE(TAG, "Failed to take picture");
            my_server.display_system_message("ERROR: Failed to take picture");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Picture taken successfully");

        // Run detection
        mu::DetectionData detection_results = my_detector.detect(pic);
        ESP_LOGI(TAG, "Detection completed successfully");

        // Check if objects were detected
        if (detection_results.detection_boxes.empty()) {
            ESP_LOGI(TAG, "No objects detected");
        } else {
            ESP_LOGI(TAG, "Objects detected: %zu", detection_results.detection_boxes.size());
            // my_server.display_system_message("Objects detected: " + std::to_string(detection_results.detection_boxes.size()));
        }

        // Update the server with detection results
        my_server.update_detection_display(detection_results);
        ESP_LOGI(TAG, "Detection results updated on server");

        // Release the picture buffer
        esp_camera_fb_return(pic);

        // Debug
        ESP_LOGI(TAG, "Free Heap: %lu bytes", esp_get_free_heap_size());
        

        // Delay for a while to reset watchdog
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

}

esp_err_t app_init()
{
    ESP_LOGI(TAG, "Initializing...");

    // Initialize the camera
    esp_err_t camera_init_err = 
        init_camera();
    if (camera_init_err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize camera");
        return camera_init_err;
    }
    ESP_LOGI(TAG, "Camera initialized successfully");

    // Initialize the WiFi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    esp_err_t wifi_err = 
        connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to connect to WiFi");
        return wifi_err;
    }
    ESP_LOGI(TAG, "Successfully connected to WiFi");

    // Initialize the web server
    esp_err_t server_init_err =
        my_server.init();
    if (server_init_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize web server");
        return server_init_err;
    }
    std::string ip_address = my_server.get_ip_address();
    ESP_LOGI(TAG, "Web server started at: %s", ip_address.c_str());
    my_server.display_system_message("Web server started. IP: http://" + ip_address);

    // Initialize the detector
    esp_err_t detector_init_err = 
        my_detector.init();
    if (detector_init_err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize detector");
        my_server.display_system_message("ERROR: Detector initialization failed");
        return detector_init_err;
    }
    ESP_LOGI(TAG, "Detector initialized successfully");

    return ESP_OK;
}
