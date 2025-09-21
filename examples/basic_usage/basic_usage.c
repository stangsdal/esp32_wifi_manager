/**
 * @file basic_usage.c
 * @brief Basic WiFi Manager usage example
 *
 * This example demonstrates the simplest way to use the WiFi Manager component:
 * 1. Initialize NVS flash
 * 2. Create WiFi Manager instance
 * 3. Auto-connect to saved WiFi or start config portal
 * 4. Handle connection results
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"

static const char *TAG = "BASIC_EXAMPLE";

/**
 * @brief Callback function called when configuration is saved
 */
void save_config_callback(void)
{
    ESP_LOGI(TAG, "Configuration saved! You can now access your custom parameters.");

    // Example: Access saved parameters (if you added any)
    // char mqtt_server[64];
    // if (wifi_manager_get_parameter_value("mqtt_server", mqtt_server, sizeof(mqtt_server)) == ESP_OK) {
    //     ESP_LOGI(TAG, "MQTT Server: %s", mqtt_server);
    // }
}

/**
 * @brief Callback function called when config portal starts (AP mode)
 */
void config_mode_callback(wifi_manager_t *wm)
{
    ESP_LOGI(TAG, "Config portal started. Connect to WiFi AP and visit http://192.168.4.1");
    ESP_LOGI(TAG, "AP Name: ESP32-WiFi-Manager");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Basic WiFi Manager Example Starting...");

    // Initialize NVS flash (required for WiFi credential storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(TAG, "NVS flash needs to be erased, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS flash initialized");

    // Create WiFi Manager instance
    wifi_manager_t *wm = wifi_manager_create();
    if (!wm)
    {
        ESP_LOGE(TAG, "Failed to create WiFi Manager instance");
        return;
    }
    ESP_LOGI(TAG, "WiFi Manager created successfully");

    // Optional: Set configuration timeout (default is 300 seconds)
    wifi_manager_set_config_portal_timeout(wm, 180); // 3 minutes

    // Optional: Set minimum signal quality (default is 8%)
    wifi_manager_set_minimum_signal_quality(wm, 20); // 20% minimum

    // Optional: Add custom configuration parameters
    wifi_manager_add_parameter(wm, "device_name", "Device Name", "ESP32-Basic", 32, NULL, NULL);
    wifi_manager_add_parameter(wm, "mqtt_server", "MQTT Server", "broker.mqtt.cool", 64, NULL, NULL);
    wifi_manager_add_parameter(wm, "mqtt_port", "MQTT Port", "1883", 6, NULL, NULL);

    // Optional: Set callbacks
    wifi_manager_set_save_config_callback(wm, save_config_callback);
    wifi_manager_set_ap_callback(wm, config_mode_callback);

    ESP_LOGI(TAG, "Starting WiFi Manager auto-connect...");

    // Auto-connect to saved WiFi credentials or start config portal
    // This is the main function that handles everything automatically
    if (wifi_manager_auto_connect(wm, "ESP32-Setup", NULL))
    {
        ESP_LOGI(TAG, "✅ Successfully connected to WiFi!");
        ESP_LOGI(TAG, "🌐 Device is now online and ready for your application");
    }
    else
    {
        ESP_LOGW(TAG, "❌ Failed to connect to WiFi");
        ESP_LOGI(TAG, "📱 Config portal should be running for manual setup");
    }

    // Your application code starts here
    ESP_LOGI(TAG, "🚀 Basic WiFi Manager example completed");
    ESP_LOGI(TAG, "💡 Add your application logic in the loop below");

    // Main application loop
    while (1)
    {
        // Your application code here
        // The WiFi Manager handles all WiFi connection management automatically

        ESP_LOGI(TAG, "Application running... (add your code here)");
        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds
    }
}