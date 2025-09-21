/**
 * @file wifi_manager_private.h
 * @brief Internal shared definitions for WiFi Manager components
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#pragma once

#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/inet.h"
#include "esp_mac.h"
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

/* ==========================================
 *             CONSTANTS
 * ========================================== */
#define WIFI_MANAGER_DEFAULT_AP_SSID "ESP32-Setup"
#define WIFI_MANAGER_DEFAULT_AP_PASS NULL      // Open AP by default
#define WIFI_MANAGER_AP_SSID "ESP32-CYD-Setup" // Legacy compatibility
#define WIFI_MANAGER_AP_PASS "12345678"        // Legacy compatibility

// Scan task notification values
#define SCAN_NOTIFICATION_START 1
#define SCAN_NOTIFICATION_COMPLETE 2

#define WIFI_MANAGER_MAX_RETRY 3
#define WIFI_MANAGER_NVS_NAMESPACE "wifi_config"
#define WIFI_MANAGER_CONFIG_NAMESPACE "app_config"
#define WIFI_MANAGER_DEFAULT_TIMEOUT 180 // 3 minutes like tzapu default
#define MAX_SCANNED_NETWORKS 20

// Configuration parameter limits
#define MAX_CONFIG_STRING_LEN 128
#define MAX_CONFIG_PARAMS 16

extern const char *TAG;

/* ==========================================
 *          DATA STRUCTURES
 * ========================================== */

// Configuration parameter types
typedef enum
{
    CONFIG_TYPE_STRING = 0,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_FLOAT
} config_param_type_t;

// Configuration parameter structure
typedef struct
{
    char key[32];                              // Parameter key (e.g., "mqtt_broker")
    char label[64];                            // Human-readable label for web UI
    config_param_type_t type;                  // Data type
    char value[MAX_CONFIG_STRING_LEN];         // String representation of value
    char default_value[MAX_CONFIG_STRING_LEN]; // Default value
    bool required;                             // Whether parameter is required
    int min_length;                            // Minimum length (for strings)
    int max_length;                            // Maximum length (for strings)
    char placeholder[64];                      // Placeholder text for web UI
    char validation_pattern[64];               // Regex pattern for validation (optional)
} config_param_t;

// Structure to hold scanned WiFi network information
typedef struct
{
    char ssid[33];             // WiFi network name
    int8_t rssi;               // Signal strength
    wifi_auth_mode_t authmode; // Security type
    bool is_hidden;            // Whether SSID is hidden
} scanned_network_t;

// WiFi Manager structure (tzapu-style)
struct wifi_manager_t
{
    // Config portal settings
    char ap_ssid[33];
    char ap_password[65];
    uint32_t config_portal_timeout;
    int minimum_signal_quality;
    bool debug_output;

    // Callbacks (tzapu-style)
    config_mode_callback_t ap_callback;
    save_config_callback_t save_callback;

    // Internal state
    esp_netif_t *sta_netif;
    esp_netif_t *ap_netif;
    httpd_handle_t server;
    wifi_status_t current_status;
    char ip_address[16];
    int retry_count;
    TimerHandle_t timeout_timer;
    bool portal_aborted;
    bool config_saved;

    // WiFi scanning
    scanned_network_t scanned_networks[MAX_SCANNED_NETWORKS];
    uint16_t scanned_count;
    bool scan_completed;
    TaskHandle_t scan_task_handle;

    // Custom configuration parameters
    config_param_t config_params[MAX_CONFIG_PARAMS];
    int config_param_count;
    bool config_portal_enabled;
};

/* ==========================================
 *          GLOBAL VARIABLES
 * ========================================== */

// Global variables for legacy API compatibility
extern esp_netif_t *sta_netif;
extern esp_netif_t *ap_netif;
extern wifi_event_callback_t user_callback;
extern wifi_status_t current_status;
extern char ip_address[16];
extern int retry_count;
extern wifi_manager_t *g_wm; // Global reference for event handler

/* ==========================================
 *          EMBEDDED WEB FILES
 * ========================================== */

// Embedded web files
extern const uint8_t setup_html_start[] asm("_binary_setup_html_start");
extern const uint8_t setup_html_end[] asm("_binary_setup_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t script_js_start[] asm("_binary_script_js_start");
extern const uint8_t script_js_end[] asm("_binary_script_js_end");
extern const uint8_t success_html_start[] asm("_binary_success_html_start");
extern const uint8_t success_html_end[] asm("_binary_success_html_end");
extern const uint8_t config_html_start[] asm("_binary_config_html_start");
extern const uint8_t config_html_end[] asm("_binary_config_html_end");

/* ==========================================
 *       INTERNAL FUNCTION PROTOTYPES
 * ========================================== */

// Core WiFi functions (wifi_manager_core.c)
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void update_status(wifi_status_t status);
void timeout_timer_callback(TimerHandle_t xTimer);

// WiFi scanning functions (wifi_manager_scan.c)
void wifi_scan_done_handler(void);
void wifi_scan_task(void *pvParameters);
void trigger_wifi_scan(wifi_manager_t *wm);

// Web server functions (wifi_manager_web.c)
esp_err_t setup_page_handler(httpd_req_t *req);
esp_err_t setup_html_handler(httpd_req_t *req);
esp_err_t style_css_handler(httpd_req_t *req);
esp_err_t script_js_handler(httpd_req_t *req);
esp_err_t success_html_handler(httpd_req_t *req);
esp_err_t config_html_handler(httpd_req_t *req);
esp_err_t connect_handler(httpd_req_t *req);
esp_err_t wifi_list_handler(httpd_req_t *req);
esp_err_t config_handler(httpd_req_t *req);
esp_err_t config_save_handler(httpd_req_t *req);
esp_err_t restart_handler(httpd_req_t *req);
esp_err_t reset_handler(httpd_req_t *req);
esp_err_t wifi_reset_handler(httpd_req_t *req);
esp_err_t start_webserver(void);
void stop_webserver(void);

// Storage functions (wifi_manager_storage.c)
esp_err_t save_wifi_credentials(const char *ssid, const char *password);
esp_err_t load_wifi_credentials(char *ssid, char *password);

// Configuration functions (wifi_manager_config.c)
esp_err_t save_config_parameters(wifi_manager_t *wm);
esp_err_t load_config_parameters(wifi_manager_t *wm);
esp_err_t reset_config_parameters(wifi_manager_t *wm);
esp_err_t add_config_parameter(wifi_manager_t *wm, const char *key, const char *label,
                               config_param_type_t type, const char *default_value,
                               bool required, const char *placeholder);
esp_err_t set_config_parameter(wifi_manager_t *wm, const char *key, const char *value);
esp_err_t get_config_parameter(wifi_manager_t *wm, const char *key, char *value, size_t value_len);
void init_default_config_parameters(wifi_manager_t *wm);