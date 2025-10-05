#ifndef TAKE_PICTURE_H
#define TAKE_PICTURE_H

#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CUSTOM_PIN 1

#ifdef CUSTOM_PIN

// Camera pins definition
#define CAM_PIN_PWDN  -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  -1
#define CAM_PIN_SIOD  39
#define CAM_PIN_SIOC  38

#define CAM_PIN_D7    18
#define CAM_PIN_D6    17
#define CAM_PIN_D5    16
#define CAM_PIN_D4    15
#define CAM_PIN_D3    7
#define CAM_PIN_D2    6
#define CAM_PIN_D1    5
#define CAM_PIN_D0    4
#define CAM_PIN_VSYNC 47
#define CAM_PIN_HREF  48
#define CAM_PIN_PCLK  45

#endif

// Key camera configuration parameters
#define CAM_XCLK_FREQ_HZ    20000000  // 20MHz XCLK frequency
#define CAM_LEDC_TIMER      LEDC_TIMER_0
#define CAM_LEDC_CHANNEL    LEDC_CHANNEL_0
#define CAM_PIXEL_FORMAT    PIXFORMAT_JPEG  // YUV422, GRAYSCALE, RGB565, JPEG
#define CAM_FRAME_SIZE      FRAMESIZE_VGA  // QQVGA-UXGA
#define CAM_JPEG_QUALITY    10  // 0-63, lower means higher quality
#define CAM_FB_COUNT        3   // Frame buffer count
#define CAM_FB_LOCATION     CAMERA_FB_IN_DRAM  // Frame buffer location
#define CAM_GRAB_MODE       CAMERA_GRAB_WHEN_EMPTY  // CAMERA_GRAB_LATEST also available

// Stream control parameters
#define CAM_STREAM_FPS      5   // Target frames per second for streaming
#define CAM_FRAME_DELAY     (1000 / CAM_STREAM_FPS)  // Delay in ms between frames

/**
 * @brief Initialize the camera
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t init_camera(void);

/**
 * @brief Take a picture using the camera
 * 
 * @return camera_fb_t* Pointer to the frame buffer or NULL if failed
 */
camera_fb_t* take_picture(void);

#ifdef __cplusplus
}
#endif

#endif // TAKE_PICTURE_H