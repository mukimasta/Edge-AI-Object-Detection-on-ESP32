#ifndef MU_SERVER_H
#define MU_SERVER_H

#include <string>
#include <vector>
#include "esp_http_server.h"
#include "mu_detector.hpp"

#define MAX_SYSTEM_MESSAGES 255

namespace mu {

/**
 * @brief Web server class for ESP32
 */
class MuServer {
public:
    /**
     * @brief Constructor takes a reference to a MuDetector
     * @param detector Reference to MuDetector instance
     */
    MuServer();
    
    /**
     * @brief Destructor automatically stops the server if running
     */
    ~MuServer();

    /**
     * @brief Initialize and start the server
     * @return ESP_OK if initialization successful, otherwise error code
     */
    esp_err_t init();

    /**
     * @brief Get ip address of the server
     * @return IP address as a string
     */
    std::string get_ip_address() const;

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Display a system message on the web page
     * @param message The message to display
     */
    void display_system_message(const std::string& message);

    /**
     * @brief Update the web page with provided detection data
     * @param detection_data The detection data to display
     * @details Updates web UI with the provided detection data
     */
    void update_detection_display(const DetectionData& detection_data);

    /**
     * @brief Display detection results as a system message
     * @details Formats current detection data as text and displays using display_system_message
     */
    // void display_detection_as_message(const DetectionData& detection_data);

private:
    httpd_handle_t server_handle_;             // HTTP server handle
    std::vector<std::string> system_messages_; // Store system messages
    DetectionData current_detection_;           // Store the most recent detection data
    
    // Static handler functions for HTTP endpoints
    static esp_err_t index_handler_(httpd_req_t *req);
    static esp_err_t stream_handler_(httpd_req_t *req);
    static esp_err_t system_messages_handler_(httpd_req_t *req);
    static esp_err_t detection_data_handler_(httpd_req_t *req);
    
    // Helper to register all URI handlers
    void register_handlers_();
    
    // Context pointer for handlers to access class instance
    static MuServer* server_instance_;
};

} // namespace mu

#endif // MU_SERVER_H

