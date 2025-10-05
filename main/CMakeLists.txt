set(requires
    pedestrian_detect
    spi_flash
    esp_wifi
    nvs_flash
    esp_event
    esp_netif
    esp_http_server
    esp_timer
    spiffs
    )

idf_component_register(SRC_DIRS . src
                       INCLUDE_DIRS src
                       REQUIRES ${requires}
                      )

# Create a directory for SPIFFS data
set(SPIFFS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
# set(SPIFFS_DST_DIR "${CMAKE_BINARY_DIR}/spiffs_image")

# Add custom command to build SPIFFS image
spiffs_create_partition_image(spiffs ${SPIFFS_SRC_DIR} FLASH_IN_PROJECT)

set(image_file "/Users/mukii/Code/esp32/fyp/main/pedestrian_detect_pico_s8_v1.espdl")
esptool_py_flash_to_partition(flash "model" "${image_file}")
