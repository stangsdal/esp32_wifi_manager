/**
 * @file wifi_manager_scan.c
 * @brief WiFi network scanning and discovery functionality
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"

/**
 * @brief Handle WiFi scan completion event - minimal processing in event context
 */
void wifi_scan_done_handler(void)
{
    if (!g_wm || !g_wm->scan_task_handle)
    {
        return;
    }

    // Just notify the scan task to process results - no data processing in event context
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(g_wm->scan_task_handle, SCAN_NOTIFICATION_COMPLETE, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);

    // Request context switch if needed
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Dedicated WiFi scan task - handles scan requests via task notifications
 * @param pvParameters Pointer to WiFiManager instance
 */
void wifi_scan_task(void *pvParameters)
{
    wifi_manager_t *wm = (wifi_manager_t *)pvParameters;

    ESP_LOGI(TAG, "WiFi scan task started");

    while (true)
    {
        // Wait for notification
        uint32_t notification_value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (notification_value == SCAN_NOTIFICATION_START && wm)
        {
            ESP_LOGI(TAG, "Scan task received start notification, starting WiFi scan...");

            // Check if we're already connected - if so, skip scanning to avoid conflicts
            if (wm->current_status == WIFI_STATUS_CONNECTED)
            {
                ESP_LOGI(TAG, "Already connected to WiFi, skipping scan");
                continue;
            }

            // Check if we're in the right mode for scanning
            wifi_mode_t mode;
            esp_err_t err = esp_wifi_get_mode(&mode);
            if (err == ESP_OK && (mode == WIFI_MODE_APSTA || mode == WIFI_MODE_STA))
            {
                // Reset scan state
                wm->scan_completed = false;
                wm->scanned_count = 0;

                // Configure scan parameters
                wifi_scan_config_t scan_config = {0};
                scan_config.show_hidden = true;
                scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
                scan_config.scan_time.active.min = 100;
                scan_config.scan_time.active.max = 300;

                // Start the scan (non-blocking)
                err = esp_wifi_scan_start(&scan_config, false);
                if (err != ESP_OK)
                {
                    ESP_LOGW(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(err));
                    wm->scan_completed = true; // Mark as completed even if failed
                }
                else
                {
                    ESP_LOGI(TAG, "WiFi scan started successfully");
                    // Wait for WIFI_EVENT_SCAN_DONE which will send SCAN_NOTIFICATION_COMPLETE
                }
            }
            else
            {
                ESP_LOGW(TAG, "WiFi not in correct mode for scanning (mode: %d)", mode);
                wm->scan_completed = true; // Mark as completed since we can't scan
            }
        }
        else if (notification_value == SCAN_NOTIFICATION_COMPLETE && wm)
        {
            ESP_LOGI(TAG, "Scan task received completion notification, processing results...");

            uint16_t ap_num = MAX_SCANNED_NETWORKS;
            wifi_ap_record_t ap_records[MAX_SCANNED_NETWORKS];

            esp_err_t err = esp_wifi_scan_get_ap_records(&ap_num, ap_records);
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG, "Failed to get scan results: %s", esp_err_to_name(err));
                wm->scanned_count = 0;
                wm->scan_completed = true;
                continue;
            }

            wm->scanned_count = ap_num;

            // Copy scan results to our structure
            for (int i = 0; i < ap_num && i < MAX_SCANNED_NETWORKS; i++)
            {
                strncpy(wm->scanned_networks[i].ssid, (char *)ap_records[i].ssid, sizeof(wm->scanned_networks[i].ssid) - 1);
                wm->scanned_networks[i].ssid[sizeof(wm->scanned_networks[i].ssid) - 1] = '\0';
                wm->scanned_networks[i].rssi = ap_records[i].rssi;
                wm->scanned_networks[i].authmode = ap_records[i].authmode;
                wm->scanned_networks[i].is_hidden = (strlen((char *)ap_records[i].ssid) == 0);
            }

            wm->scan_completed = true;

            ESP_LOGI(TAG, "WiFi scan completed. Found %d networks", ap_num);
        }
    }
}

/**
 * @brief Trigger a WiFi scan using task notification
 * @param wm WiFiManager instance
 */
void trigger_wifi_scan(wifi_manager_t *wm)
{
    if (wm && wm->scan_task_handle)
    {
        ESP_LOGI(TAG, "Triggering WiFi scan...");
        xTaskNotify(wm->scan_task_handle, SCAN_NOTIFICATION_START, eSetValueWithOverwrite);
    }
    else
    {
        ESP_LOGW(TAG, "Cannot trigger scan - WiFiManager or scan task not available");
    }
}