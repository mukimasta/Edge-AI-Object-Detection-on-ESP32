#include "mu_server.hpp"
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_spiffs.h>
#include <sys/param.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <time.h>

static const char* TAG = "MuServer";

namespace mu {

// Initialize static member
MuServer* MuServer::server_instance_ = nullptr;

MuServer::MuServer() : server_handle_(nullptr) {
    // Set the instance pointer to this
    server_instance_ = this;
}

MuServer::~MuServer() {
    stop();
    server_instance_ = nullptr;
}

esp_err_t MuServer::init() {
    ESP_LOGI(TAG, "Starting server initialization");
    
    // ----------- SNTP 时间同步 -------------
    ESP_LOGI(TAG, "Initializing SNTP for time sync");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    // 等待时间同步完成
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGW(TAG, "System time not set after SNTP sync attempts");
    } else {
        ESP_LOGI(TAG, "System time is set: %04d-%02d-%02d %02d:%02d:%02d", 
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    // ----------- SNTP 时间同步结束 -------------
    
    // Initialize SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = false
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }
    
    // Start HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;
    
    ret = httpd_start(&server_handle_, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %d", ret);
        return ret;
    }
    
    // Register URI handlers
    register_handlers_();
    
    ESP_LOGI(TAG, "Server started successfully");
    return ESP_OK;
}

void MuServer::stop() {
    if (server_handle_) {
        httpd_stop(server_handle_);
        server_handle_ = nullptr;
        ESP_LOGI(TAG, "Server stopped");
    }
    
    // Unmount SPIFFS
    esp_vfs_spiffs_unregister("spiffs");
}

std::string MuServer::get_ip_address() const {
    // Use esp_netif API (newer approach) instead of deprecated tcpip_adapter
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    
    if (netif == NULL) {
        return "0.0.0.0"; // Return a default if no interface found
    }
    
    esp_netif_get_ip_info(netif, &ip_info);
    char ip_str[16];
    sprintf(ip_str, "%lu.%lu.%lu.%lu", 
            (ip_info.ip.addr & 0xff), 
            ((ip_info.ip.addr >> 8) & 0xff), 
            ((ip_info.ip.addr >> 16) & 0xff), 
            ((ip_info.ip.addr >> 24) & 0xff));
    return std::string(ip_str);
}

void MuServer::display_system_message(const std::string& message) {
    ESP_LOGI(TAG, "System message: %s", message.c_str());
    // Time Zone Set
    setenv("TZ", "CST-8", 1);
    tzset();
    // Add timestamp to the message
    char timestamp[32];
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", &timeinfo);
    std::string timestamped_message = std::string(timestamp) + message;
    // Add to messages vector
    system_messages_.push_back(timestamped_message);
    // Limit the number of messages
    if (system_messages_.size() > MAX_SYSTEM_MESSAGES) {
        system_messages_.erase(system_messages_.begin());
    }
}

void MuServer::update_detection_display(const DetectionData& detection_data) {
    current_detection_ = detection_data;
}

esp_err_t MuServer::index_handler_(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving index page");
    
    // Set content type
    httpd_resp_set_type(req, "text/html");
    
    // Only use the path that works according to logs
    FILE* file = fopen("/spiffs/index.html", "r");
    
    if (!file) {
        ESP_LOGE(TAG, "Failed to open /spiffs/index.html");
        
        // Send a simple hardcoded HTML as fallback
        const char* fallback_html = "<html><body><h1>ESP32 Human Detection System</h1><p>Could not load index.html</p></body></html>";
        httpd_resp_send(req, fallback_html, strlen(fallback_html));
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Successfully opened /spiffs/index.html");
    
    // Read and send file in chunks
    char buf[1024];
    size_t read_bytes;
    
    while ((read_bytes = fread(buf, 1, sizeof(buf), file)) > 0) {
        httpd_resp_send_chunk(req, buf, read_bytes);
    }
    
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    
    return ESP_OK;
}

esp_err_t MuServer::stream_handler_(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    
    if (!server_instance_ || !server_instance_->current_detection_.pic) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, "No detection image available yet", -1);
        return ESP_OK;
    }
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, reinterpret_cast<const char*>(server_instance_->current_detection_.pic->buf),
                    server_instance_->current_detection_.pic->len);
    return ESP_OK;
}

esp_err_t MuServer::system_messages_handler_(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving system messages");
    
    // Set content type to JSON
    httpd_resp_set_type(req, "application/json");
    
    // Add CORS headers to allow any origin
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    // Get instance for accessing messages
    MuServer* instance = server_instance_;
    if (!instance) {
        ESP_LOGE(TAG, "Server instance is null");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server error");
        return ESP_FAIL;
    }
    
    // Create JSON array of messages
    
    std::string json = "[";
    
    // If there are no messages, add a default one
    if (instance->system_messages_.empty()) {
        json += "\"[System] No messages yet.\"";
    } else {
        for (size_t i = 0; i < instance->system_messages_.size(); i++) {
            // Add quotes around message and escape special characters
            json += "\"";
            for (char c : instance->system_messages_[i]) {
                if (c == '\"') json += "\\\"";
                else if (c == '\\') json += "\\\\";
                else if (c == '\n') json += "\\n";
                else if (c == '\r') json += "\\r";
                else if (c == '\t') json += "\\t";
                else json += c;
            }
            json += "\"";
            
            // Add comma if not the last element
            if (i < instance->system_messages_.size() - 1) {
                json += ",";
            }
        }
    }
    
    json += "]";
    
    // Send the JSON response
    httpd_resp_send(req, json.c_str(), json.length());
    
    return ESP_OK;
}

esp_err_t MuServer::detection_data_handler_(httpd_req_t *req) {
    if (!server_instance_) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\": \"Server instance not initialized\"}", -1);
        return ESP_FAIL;
    }

    const DetectionData& detection = server_instance_->current_detection_;
    std::string json = "{";

    // Add timestamp
    json += "\"timestamp\": " + std::to_string(detection.timestamp) + ",";

    // Add image information
    if (detection.pic) {
        json += "\"image\": {";
        json += "\"width\": " + std::to_string(detection.pic->width) + ",";
        json += "\"height\": " + std::to_string(detection.pic->height) + ",";
        json += "\"format\": \"jpeg\"";
        json += "},";
    } else {
        json += "\"image\": null,";
    }

    // Add detection boxes
    json += "\"detections\": [";
    if (!detection.detection_boxes.empty()) {
        for (size_t i = 0; i < detection.detection_boxes.size(); ++i) {
            const auto& box = detection.detection_boxes[i];
            
            json += "{";
            json += "\"class\": \"" + std::string(box.class_name) + "\",";
            json += "\"score\": " + std::to_string(box.score) + ",";
            json += "\"bbox_absolute\": [" + 
                    std::to_string(static_cast<int>(box.x1)) + ", " + 
                    std::to_string(static_cast<int>(box.y1)) + ", " + 
                    std::to_string(static_cast<int>(box.x2)) + ", " + 
                    std::to_string(static_cast<int>(box.y2)) + "]";
            json += "}";
            
            if (i < detection.detection_boxes.size() - 1) {
                json += ",";
            }
        }
    }
    json += "]";
    json += "}";

    // Send the JSON response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

void MuServer::register_handlers_() {
    // Register index page handler
    httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler_,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_handle_, &uri_get);
    
    // Add handlers for other endpoints
    httpd_uri_t uri_stream = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler_,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_handle_, &uri_stream);
    
    httpd_uri_t uri_system_messages = {
        .uri       = "/system-messages",
        .method    = HTTP_GET,
        .handler   = system_messages_handler_,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_handle_, &uri_system_messages);
    
    httpd_uri_t uri_detection_data = {
        .uri       = "/detection-data",
        .method    = HTTP_GET,
        .handler   = detection_data_handler_,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server_handle_, &uri_detection_data);
}

} // namespace mu
