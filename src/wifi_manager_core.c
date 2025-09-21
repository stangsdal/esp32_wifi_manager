/**
 * @file wifi_manager_core.c
 * @brief Core WiFi event handling and status management
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"

// Global variables (declared in private header)
esp_netif_t *sta_netif = NULL;
esp_netif_t *ap_netif = NULL;
wifi_event_callback_t user_callback = NULL;
wifi_status_t current_status = WIFI_STATUS_DISCONNECTED;
char ip_address[16] = {0};
int retry_count = 0;
wifi_manager_t *g_wm = NULL; // Global reference for event handler

const char *TAG = "wifi_manager";

/**
 * @brief Update WiFi status and notify if callback registered
 */
void update_status(wifi_status_t status)
{
    current_status = status;
    if (g_wm)
    {
        g_wm->current_status = status;
    }

    ESP_LOGI(TAG, "Status updated to: %d", status);

    // Call user callback if registered
    if (user_callback)
    {
        user_callback(status, (status == WIFI_STATUS_CONNECTED) ? ip_address : NULL);
    }
}

/**
 * @brief Timeout timer callback for configuration portal
 */
void timeout_timer_callback(TimerHandle_t xTimer)
{
    wifi_manager_t *wm = (wifi_manager_t *)pvTimerGetTimerID(xTimer);
    if (wm)
    {
        ESP_LOGI(TAG, "Configuration portal timeout reached");
        wm->portal_aborted = true;
    }
}

/**
 * @brief Main WiFi event handler
 */
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Station started");
            break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
            ESP_LOGI(TAG, "Connected to WiFi network: %s", event->ssid);
            update_status(WIFI_STATUS_CONNECTING);
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Disconnected from WiFi (reason: %d)", event->reason);

            if (g_wm)
            {
                g_wm->retry_count++;
                if (g_wm->retry_count < WIFI_MANAGER_MAX_RETRY)
                {
                    ESP_LOGI(TAG, "Retrying connection... (%d/%d)", g_wm->retry_count, WIFI_MANAGER_MAX_RETRY);
                    esp_wifi_connect();
                    update_status(WIFI_STATUS_CONNECTING);
                }
                else
                {
                    ESP_LOGW(TAG, "Max connection retries reached");
                    update_status(WIFI_STATUS_DISCONNECTED);
                    g_wm->retry_count = 0;
                }
            }
            else
            {
                // Legacy global variable handling
                retry_count++;
                if (retry_count < WIFI_MANAGER_MAX_RETRY)
                {
                    ESP_LOGI(TAG, "Retrying connection... (%d/%d)", retry_count, WIFI_MANAGER_MAX_RETRY);
                    esp_wifi_connect();
                    update_status(WIFI_STATUS_CONNECTING);
                }
                else
                {
                    ESP_LOGW(TAG, "Max connection retries reached");
                    update_status(WIFI_STATUS_DISCONNECTED);
                    retry_count = 0;
                }
            }
            break;
        }

        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "Access Point started");
            break;

        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "Access Point stopped");
            break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "Station connected to AP: " MACSTR ", AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "Station disconnected from AP: " MACSTR ", AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGI(TAG, "WiFi scan completed");
            wifi_scan_done_handler();
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

            // Format IP address
            snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&event->ip_info.ip));

            if (g_wm)
            {
                strncpy(g_wm->ip_address, ip_address, sizeof(g_wm->ip_address) - 1);
                g_wm->ip_address[sizeof(g_wm->ip_address) - 1] = '\0';
                g_wm->retry_count = 0;
            }
            else
            {
                retry_count = 0;
            }

            ESP_LOGI(TAG, "Got IP address: %s", ip_address);
            update_status(WIFI_STATUS_CONNECTED);
            break;
        }

        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "Lost IP address");
            memset(ip_address, 0, sizeof(ip_address));
            if (g_wm)
            {
                memset(g_wm->ip_address, 0, sizeof(g_wm->ip_address));
            }
            update_status(WIFI_STATUS_DISCONNECTED);
            break;
        }
    }
}