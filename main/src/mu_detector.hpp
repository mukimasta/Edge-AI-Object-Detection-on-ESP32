#pragma once

#include <vector>
#include <cstdint>
#include <memory>

#include <esp_err.h>

#include "pedestrian_detect.hpp"
#include "esp_camera.h"

namespace mu {

/**
 * @brief Structure to represent a detection bounding box
 */
struct DetectionBox {
    float x1;       ///< x-coordinate of top-left corner
    float y1;       ///< y-coordinate of top-left corner
    float x2;       ///< x-coordinate of bottom-right corner
    float y2;       ///< y-coordinate of bottom-right corner
    float score;    ///< confidence score of the detection
    char class_name[16];   ///< name of the detected object (e.g., "person") as C-style string
};

/**
 * @brief Structure to hold detection results data
 */
struct DetectionData {
    uint64_t timestamp;                  ///< Current time in milliseconds
    camera_fb_t* pic;                         ///< Pointer to the esp_camera image data
    std::vector<DetectionBox> detection_boxes;     ///< Detection boxes results
};

/**
 * @brief Pedestrian detector class for ESP32
 */
class MuDetector {
public:
    MuDetector();
    ~MuDetector();

    /**
     * @brief Initialize the detector
     * @return ESP_OK if initialization successful, ESP_FAIL otherwise
     */
    esp_err_t init();
    
    /**
     * @brief Perform detection on an input image
     * @param pic Pointer to the image data
     * @return Detection results
     */
    DetectionData detect(camera_fb_t *pic);
    
    /**
     * @brief Get the current detection data (const version)
     * @return Const reference to the current detection data
     */
    const DetectionData& getCurrentDetection() const { return current_detection_; }
    
    /**
     * @brief Get the detection history
     * @return Const reference to the detection history
     */
    // const std::vector<DetectionData>& getDetectionHistory() const { return detection_history_; }

private:
    PedestrianDetect* my_detect_;
    DetectionData current_detection_;
    // std::vector<DetectionData> detection_history_;
};

} // namespace mu
