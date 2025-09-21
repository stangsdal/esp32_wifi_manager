/**
 * @file advanced_features.c
 * @brief Advanced WiFi Manager features demonstration
 *
 * This example demonstrates advanced features of the WiFi Manager:
 * - Custom configuration parameters
 * - Callback functions for events
 * - Configuration portal customization
 * - Parameter validation and retrieval
 * - Status monitoring and reconnection handling
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi_manager.h"

static const char *TAG = "ADVANCED_EXAMPLE";

// Global WiFi Manager instance
static wifi_manager_t *g_wm = NULL;

// Event group for application synchronization
static EventGroupHandle_t app_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define CONFIG_SAVED_BIT BIT1

// Application configuration structure
typedef struct
{
    char mqtt_server[64];
    int mqtt_port;
    char mqtt_username[32];
    char mqtt_password[32];
    char device_name[32];
    int update_interval;
    bool debug_enabled;
} app_config_t;

static app_config_t app_config = {0};

/**
 * @brief Callback function called when WiFi configuration is saved
 * This is where you would typically save your custom parameters
 */
void save_config_callback(void)
{
    ESP_LOGI(TAG, "🔄 Configuration saved! Processing custom parameters...");

    // Retrieve and validate custom parameters
    char temp_str[64];

    // MQTT Server
    if (wifi_manager_get_parameter_value("mqtt_server", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        strncpy(app_config.mqtt_server, temp_str, sizeof(app_config.mqtt_server) - 1);
        ESP_LOGI(TAG, "📡 MQTT Server: %s", app_config.mqtt_server);
    }

    // MQTT Port
    if (wifi_manager_get_parameter_value("mqtt_port", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        app_config.mqtt_port = atoi(temp_str);
        if (app_config.mqtt_port < 1 || app_config.mqtt_port > 65535)
        {
            ESP_LOGW(TAG, "⚠️ Invalid MQTT port, using default 1883");
            app_config.mqtt_port = 1883;
        }
        ESP_LOGI(TAG, "🔌 MQTT Port: %d", app_config.mqtt_port);
    }

    // MQTT Username
    if (wifi_manager_get_parameter_value("mqtt_username", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        strncpy(app_config.mqtt_username, temp_str, sizeof(app_config.mqtt_username) - 1);
        ESP_LOGI(TAG, "👤 MQTT Username: %s", app_config.mqtt_username);
    }

    // MQTT Password (don't log the actual password for security)
    if (wifi_manager_get_parameter_value("mqtt_password", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        strncpy(app_config.mqtt_password, temp_str, sizeof(app_config.mqtt_password) - 1);
        ESP_LOGI(TAG, "🔐 MQTT Password: %s", strlen(app_config.mqtt_password) > 0 ? "[SET]" : "[EMPTY]");
    }

    // Device Name
    if (wifi_manager_get_parameter_value("device_name", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        strncpy(app_config.device_name, temp_str, sizeof(app_config.device_name) - 1);
        ESP_LOGI(TAG, "📱 Device Name: %s", app_config.device_name);
    }

    // Update Interval
    if (wifi_manager_get_parameter_value("update_interval", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        app_config.update_interval = atoi(temp_str);
        if (app_config.update_interval < 1)
        {
            ESP_LOGW(TAG, "⚠️ Invalid update interval, using default 30s");
            app_config.update_interval = 30;
        }
        ESP_LOGI(TAG, "⏱️ Update Interval: %ds", app_config.update_interval);
    }

    // Debug Flag
    if (wifi_manager_get_parameter_value("enable_debug", temp_str, sizeof(temp_str)) == ESP_OK)
    {
        app_config.debug_enabled = (strcmp(temp_str, "true") == 0);
        ESP_LOGI(TAG, "🐛 Debug Mode: %s", app_config.debug_enabled ? "ENABLED" : "DISABLED");

        // Adjust log levels based on debug setting
        if (app_config.debug_enabled)
        {
            esp_log_level_set("*", ESP_LOG_DEBUG);
            ESP_LOGI(TAG, "🔍 Debug logging enabled for all components");
        }
        else
        {
            esp_log_level_set("*", ESP_LOG_INFO);
        }
    }

    // Set event bit to notify main application
    xEventGroupSetBits(app_event_group, CONFIG_SAVED_BIT);

    ESP_LOGI(TAG, "✅ All configuration parameters processed successfully!");
}

/**
 * @brief Callback function called when config portal starts (AP mode)
 */
void config_mode_callback(wifi_manager_t *wm)
{
    ESP_LOGI(TAG, "🔧 Configuration portal started!");
    ESP_LOGI(TAG, "📱 Connect to WiFi network: ESP32-Advanced-Setup");
    ESP_LOGI(TAG, "🌐 Open browser and go to: http://192.168.4.1");
    ESP_LOGI(TAG, "⏰ Portal will timeout after 5 minutes if no configuration is saved");

    // You could add LED indication, display message, etc. here
    // Example: turn on configuration LED
    // gpio_set_level(CONFIG_LED_PIN, 1);
}

/**
 * @brief Callback function called when WiFi connects successfully
 */
void wifi_connected_callback(void)
{
    ESP_LOGI(TAG, "🎉 WiFi connected successfully!");

    // Get and display connection information
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "📶 Connected to: %s", (char *)ap_info.ssid);
        ESP_LOGI(TAG, "📡 Signal strength: %d dBm", ap_info.rssi);
        ESP_LOGI(TAG, "🔐 Security: %s",
                 ap_info.authmode == WIFI_AUTH_OPEN ? "Open" : ap_info.authmode == WIFI_AUTH_WPA2_PSK ? "WPA2"
                                                                                                      : "Other");
    }

    // Set event bit to notify main application
    xEventGroupSetBits(app_event_group, WIFI_CONNECTED_BIT);

    // You could start other network services here
    // Example: start MQTT client, HTTP server, etc.
}

/**
 * @brief Load saved configuration from NVS
 */
esp_err_t load_app_config(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("app_config", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS handle for app config: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = sizeof(app_config_t);
    err = nvs_get_blob(nvs_handle, "config", &app_config, &required_size);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "📂 Loaded saved application configuration");
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "📝 No saved config found, using defaults");
        // Set default values
        strcpy(app_config.mqtt_server, "broker.mqtt.cool");
        app_config.mqtt_port = 1883;
        strcpy(app_config.device_name, "ESP32-Advanced");
        app_config.update_interval = 30;
        app_config.debug_enabled = false;
    }
    else
    {
        ESP_LOGE(TAG, "Error reading app config from NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Save application configuration to NVS
 */
esp_err_t save_app_config(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("app_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle for app config: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(nvs_handle, "config", &app_config, sizeof(app_config_t));
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "💾 Application configuration saved to NVS");
        }
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Monitor WiFi status and handle reconnections
 */
void wifi_monitor_task(void *pvParameters)
{
    const TickType_t check_interval = pdMS_TO_TICKS(5000); // Check every 5 seconds

    while (1)
    {
        // Get current WiFi status
        wifi_manager_status_t status = wifi_manager_get_status();

        switch (status)
        {
        case WIFI_STATUS_CONNECTED:
            ESP_LOGD(TAG, "📶 WiFi status: Connected");
            break;

        case WIFI_STATUS_CONNECTING:
            ESP_LOGD(TAG, "🔄 WiFi status: Connecting...");
            break;

        case WIFI_STATUS_DISCONNECTED:
            ESP_LOGW(TAG, "❌ WiFi status: Disconnected - attempting reconnection");
            // WiFi Manager handles automatic reconnection
            break;

        case WIFI_STATUS_AP_MODE:
            ESP_LOGI(TAG, "🏠 WiFi status: AP Mode (Config Portal)");
            break;

        default:
            ESP_LOGD(TAG, "🤔 WiFi status: Unknown");
            break;
        }

        vTaskDelay(check_interval);
    }
}

/**
 * @brief Simulated application task that uses the configuration
 */
void application_task(void *pvParameters)
{
    const TickType_t update_interval = pdMS_TO_TICKS(app_config.update_interval * 1000);

    ESP_LOGI(TAG, "🚀 Application task started with %ds update interval", app_config.update_interval);

    while (1)
    {
        // Wait for WiFi connection
        EventBits_t bits = xEventGroupWaitBits(app_event_group, WIFI_CONNECTED_BIT,
                                               pdFALSE, pdFALSE, portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "📊 Application running with configuration:");
            ESP_LOGI(TAG, "  📡 MQTT Server: %s:%d", app_config.mqtt_server, app_config.mqtt_port);
            ESP_LOGI(TAG, "  📱 Device Name: %s", app_config.device_name);
            ESP_LOGI(TAG, "  ⏱️ Update Interval: %ds", app_config.update_interval);
            ESP_LOGI(TAG, "  🐛 Debug Mode: %s", app_config.debug_enabled ? "ON" : "OFF");

            // Here you would implement your actual application logic:
            // - Connect to MQTT broker using app_config.mqtt_server
            // - Publish sensor data every app_config.update_interval seconds
            // - Use app_config.device_name as client ID
            // - Enable/disable debug logging based on app_config.debug_enabled

            vTaskDelay(update_interval);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "🎯 Advanced WiFi Manager Example Starting...");
    ESP_LOGI(TAG, "📚 This example demonstrates advanced features:");
    ESP_LOGI(TAG, "   - Custom configuration parameters");
    ESP_LOGI(TAG, "   - Event callbacks and status monitoring");
    ESP_LOGI(TAG, "   - Configuration persistence in NVS");
    ESP_LOGI(TAG, "   - Real-time parameter validation");

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(TAG, "🧹 Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "💾 NVS flash initialized");

    // Create event group for app synchronization
    app_event_group = xEventGroupCreate();

    // Load saved application configuration
    load_app_config();

    // Create WiFi Manager instance
    g_wm = wifi_manager_create();
    if (!g_wm)
    {
        ESP_LOGE(TAG, "❌ Failed to create WiFi Manager instance");
        return;
    }
    ESP_LOGI(TAG, "✅ WiFi Manager created successfully");

    // Configure WiFi Manager settings
    wifi_manager_set_config_portal_timeout(g_wm, 300); // 5 minutes
    wifi_manager_set_minimum_signal_quality(g_wm, 15); // 15% minimum signal

    // Add custom configuration parameters with current values as defaults
    wifi_manager_add_parameter(g_wm, "mqtt_server", "MQTT Broker",
                               app_config.mqtt_server, 64, NULL, NULL);

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", app_config.mqtt_port);
    wifi_manager_add_parameter(g_wm, "mqtt_port", "MQTT Port",
                               port_str, 6, NULL, NULL);

    wifi_manager_add_parameter(g_wm, "mqtt_username", "MQTT Username",
                               app_config.mqtt_username, 32, NULL, NULL);

    wifi_manager_add_parameter(g_wm, "mqtt_password", "MQTT Password",
                               "", 32, "type='password'", NULL);

    wifi_manager_add_parameter(g_wm, "device_name", "Device Name",
                               app_config.device_name, 32, NULL, NULL);

    char interval_str[8];
    snprintf(interval_str, sizeof(interval_str), "%d", app_config.update_interval);
    wifi_manager_add_parameter(g_wm, "update_interval", "Update Interval (seconds)",
                               interval_str, 6, "type='number' min='1' max='3600'", NULL);

    wifi_manager_add_parameter(g_wm, "enable_debug", "Enable Debug Logging",
                               app_config.debug_enabled ? "true" : "false", 5,
                               "type='checkbox'", NULL);

    // Set callback functions
    wifi_manager_set_save_config_callback(g_wm, save_config_callback);
    wifi_manager_set_ap_callback(g_wm, config_mode_callback);
    // Note: wifi_connected_callback would need to be implemented in the component
    // This is just for demonstration purposes

    ESP_LOGI(TAG, "🔧 WiFi Manager configured with advanced features");
    ESP_LOGI(TAG, "🚀 Starting auto-connect process...");

    // Start WiFi Manager auto-connect
    if (wifi_manager_auto_connect(g_wm, "ESP32-Advanced-Setup", NULL))
    {
        ESP_LOGI(TAG, "🎉 Successfully connected to WiFi!");
        wifi_connected_callback(); // Manual call since callback isn't in component yet
    }
    else
    {
        ESP_LOGW(TAG, "⚠️ Failed to connect to WiFi");
        ESP_LOGI(TAG, "🔧 Config portal should be running for manual setup");
    }

    // Start monitoring and application tasks
    xTaskCreate(wifi_monitor_task, "wifi_monitor", 2048, NULL, 2, NULL);
    xTaskCreate(application_task, "app_task", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "🎯 Advanced WiFi Manager example setup completed!");
    ESP_LOGI(TAG, "💡 Monitor the logs to see configuration and status updates");

    // Main loop - could be used for other housekeeping tasks
    while (1)
    {
        // Check if configuration was saved and save app config
        EventBits_t bits = xEventGroupWaitBits(app_event_group, CONFIG_SAVED_BIT,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));

        if (bits & CONFIG_SAVED_BIT)
        {
            ESP_LOGI(TAG, "💾 Saving application configuration to NVS...");
            save_app_config();
        }

        // Other housekeeping tasks could go here
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}