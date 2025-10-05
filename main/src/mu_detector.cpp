#include "mu_detector.hpp"
#include <cstring>
#include <sys/time.h>

static const char* TAG = "MuDetector";

namespace mu {

// Helper function to get current timestamp in milliseconds
static uint64_t get_current_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

MuDetector::MuDetector() : my_detect_(nullptr) {
    ESP_LOGI(TAG, "Pedestrian detection model initialized");
}

MuDetector::~MuDetector() {
    // Clean up resources
    if (my_detect_) {
        delete my_detect_;
        my_detect_ = nullptr;
    }
}

esp_err_t MuDetector::init() {
    // Initialize the pedestrian detector
    my_detect_ = new PedestrianDetect();
    if (!my_detect_) {
        ESP_LOGE(TAG, "Failed to allocate PedestrianDetect");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Pedestrian detector initialized successfully");
    return ESP_OK;
}

DetectionData MuDetector::detect(camera_fb_t *pic) {
    // Create a new detection data structure
    DetectionData result;
    result.timestamp = get_current_timestamp();
    result.pic = pic;  // Store pointer to camera frame buffer
    // Ensure detection_boxes is empty at the start
    result.detection_boxes.clear();

    dl::image::jpeg_img_t jpeg_img = {
        .data = pic->buf,
        .width = int(pic->width),
        .height = int(pic->height),
        .data_size = pic->len,
    };
    
    dl::image::img_t img;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    // uint64_t infer_start = get_current_timestamp(); // 推理前时间戳
    if (sw_decode_jpeg(jpeg_img, img, true) == ESP_OK) {
        // Detect
        auto &detect_results = my_detect_->run(img);
        // uint64_t infer_end = get_current_timestamp(); // 推理后时间戳
        // ESP_LOGI(TAG, "Model inference latency: %llu ms", (infer_end - infer_start));
        heap_caps_free(img.data); // IMPORTANT!! Memory leak point
        
        // Check if we have any valid detections above threshold
        bool has_valid_detection = false;
        for (const auto &res : detect_results) {
            if (res.score > 0.3) { // 只包括置信度较高的结果
                has_valid_detection = true;
                break;
            }
        }
        
        if (!has_valid_detection) {
            ESP_LOGI(TAG, "No pedestrians detected in this frame");
            // result.detection_boxes is already empty, we just log the information
        } else {
            // Process detection results and convert to DetectionBox format
            for (const auto &res : detect_results) {
                if (res.score > 0.3) { // 只包括置信度较高的结果
                    DetectionBox box;
                    box.x1 = static_cast<float>(res.box[0]);
                    box.y1 = static_cast<float>(res.box[1]);
                    box.x2 = static_cast<float>(res.box[2]);
                    box.y2 = static_cast<float>(res.box[3]);
                    box.score = res.score;
                    
                    // Set class name as "person"
                    strncpy(box.class_name, "person", sizeof(box.class_name) - 1);
                    box.class_name[sizeof(box.class_name) - 1] = '\0'; // Ensure null termination
                    
                    // Add to detection boxes vector
                    result.detection_boxes.push_back(box);
                    
                    ESP_LOGI(TAG, "Pedestrian detected [score: %.2f, x1: %.1f, y1: %.1f, x2: %.1f, y2: %.1f]",
                            box.score, box.x1, box.y1, box.x2, box.y2);
                }
            }
            
            // Log the detection summary
            ESP_LOGI(TAG, "Detection completed: found %d pedestrians", result.detection_boxes.size());
        }
    } else {
        ESP_LOGW(TAG, "Failed to decode JPEG image for detection");
    }
    
    // Update the current detection and return the result
    current_detection_ = result;
    return result;
}

} // namespace mu
