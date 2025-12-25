/**
 * @file wifi_manager_api.c
 * @brief Public API functions for WiFi Manager
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"

/* ==========================================
 *          TZAPU-STYLE API FUNCTIONS
 * ========================================== */

/**
 * @brief Create a new WiFiManager instance (tzapu-style API)
 * @return Pointer to WiFiManager instance or NULL on failure
 */
wifi_manager_t *wifi_manager_create(void)
{
    wifi_manager_t *wm = malloc(sizeof(wifi_manager_t));
    if (!wm)
    {
        ESP_LOGE(TAG, "Failed to allocate WiFiManager");
        return NULL;
    }

    // Initialize with defaults
    strncpy(wm->ap_ssid, WIFI_MANAGER_DEFAULT_AP_SSID, sizeof(wm->ap_ssid) - 1);
    wm->ap_ssid[sizeof(wm->ap_ssid) - 1] = '\0';
    memset(wm->ap_password, 0, sizeof(wm->ap_password));
    wm->config_portal_timeout = WIFI_MANAGER_DEFAULT_TIMEOUT;
    wm->minimum_signal_quality = 8; // tzapu default
    wm->debug_output = true;
    wm->ap_callback = NULL;
    wm->save_callback = NULL;
    wm->sta_netif = NULL;
    wm->ap_netif = NULL;
    wm->server = NULL;
    wm->current_status = WIFI_STATUS_DISCONNECTED;
    memset(wm->ip_address, 0, sizeof(wm->ip_address));
    wm->retry_count = 0;
    wm->timeout_timer = NULL;
    wm->portal_aborted = false;
    wm->config_saved = false;

    // Initialize WiFi scan fields
    wm->scanned_count = 0;
    wm->scan_completed = false;
    wm->scan_task_handle = NULL;
    memset(wm->scanned_networks, 0, sizeof(wm->scanned_networks));

    // Initialize configuration parameters
    init_default_config_parameters(wm);

    // Initialize TCP/IP stack and WiFi subsystem
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize network interface: %s", esp_err_to_name(ret));
        free(wm);
        return NULL;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        free(wm);
        return NULL;
    }

    // Create network interfaces
    wm->sta_netif = esp_netif_create_default_wifi_sta();
    wm->ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        free(wm);
        return NULL;
    }

    // Register event handlers
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
    }
    
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
    }

    // Create the WiFi scan task
    BaseType_t task_result = xTaskCreate(
        wifi_scan_task,
        "wifi_scan_task",
        4096, // Stack size - sufficient for WiFi operations
        wm,   // Parameters - pass WiFiManager instance
        3,    // Priority - lower than main task but higher than idle
        &wm->scan_task_handle);

    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create WiFi scan task");
        free(wm);
        return NULL;
    }

    if (wm->debug_output)
    {
        ESP_LOGI(TAG, "WiFiManager created");
    }

    // Set global reference for event handler
    g_wm = wm;

    return wm;
}

/**
 * @brief Destroy WiFiManager instance and cleanup resources
 */
void wifi_manager_destroy(wifi_manager_t *wm)
{
    if (!wm)
        return;

    if (wm->timeout_timer)
    {
        xTimerDelete(wm->timeout_timer, portMAX_DELAY);
    }

    // Clean up scan task
    if (wm->scan_task_handle)
    {
        vTaskDelete(wm->scan_task_handle);
        wm->scan_task_handle = NULL;
    }

    // Stop web server
    stop_webserver();

    // Clear global reference
    if (g_wm == wm)
    {
        g_wm = NULL;
    }

    free(wm);
}

/**
 * @brief Auto-connect to saved WiFi or start config portal
 */
bool wifi_manager_auto_connect(wifi_manager_t *wm, const char *ap_name, const char *ap_password)
{
    if (!wm)
        return false;

    ESP_LOGI(TAG, "Starting WiFiManager auto-connect...");

    // Try to load and connect to saved WiFi first
    char saved_ssid[32] = {0};
    char saved_password[64] = {0};

    if (load_wifi_credentials(saved_ssid, saved_password) == ESP_OK && strlen(saved_ssid) > 0)
    {
        ESP_LOGI(TAG, "Found saved WiFi credentials for: %s", saved_ssid);

        // Try to connect using the legacy start function which handles STA mode
        esp_err_t ret = wifi_manager_start();
        if (ret == ESP_OK)
        {
            // Give time for connection attempt with retries (max 3 retries * ~5s each = ~15s + buffer)
            const int max_wait_time_ms = 20000; // 20 seconds
            const int check_interval_ms = 500;  // Check every 500ms
            int elapsed_time_ms = 0;

            while (elapsed_time_ms < max_wait_time_ms && current_status != WIFI_STATUS_CONNECTED && current_status != WIFI_STATUS_DISCONNECTED)
            {
                vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
                elapsed_time_ms += check_interval_ms;
            }

            // Check if we successfully connected
            if (current_status == WIFI_STATUS_CONNECTED)
            {
                ESP_LOGI(TAG, "Successfully connected to saved WiFi after %d ms", elapsed_time_ms);
                return true;
            }
            else
            {
                ESP_LOGW(TAG, "Connection failed or timed out after %d ms (status: %d)", elapsed_time_ms, current_status);
            }
        }
        ESP_LOGW(TAG, "Failed to connect to saved WiFi, starting config portal");
    }
    else
    {
        ESP_LOGI(TAG, "No saved WiFi credentials found, starting config portal");
    }

    // Start configuration portal using legacy implementation
    return wifi_manager_start_config_portal(wm, ap_name ? ap_name : wm->ap_ssid,
                                            ap_password ? ap_password : (strlen(wm->ap_password) > 0 ? wm->ap_password : NULL));
}

/**
 * @brief Start configuration portal with specified AP credentials
 */
bool wifi_manager_start_config_portal(wifi_manager_t *wm, const char *ap_name, const char *ap_password)
{
    if (!wm)
        return false;

    ESP_LOGI(TAG, "Starting config portal: %s", ap_name ? ap_name : wm->ap_ssid);

    wm->portal_aborted = false;
    wm->config_saved = false;
    wm->current_status = WIFI_STATUS_CONFIG_PORTAL;

    // Call config mode callback if set
    if (wm->ap_callback)
    {
        wm->ap_callback(wm);
    }

    // Stop any existing WiFi and start in AP mode using legacy implementation
    esp_wifi_stop();

    // Configure AP mode with the provided credentials
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    // Set SSID
    const char *ssid = ap_name ? ap_name : wm->ap_ssid;
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);

    // Set password if provided
    const char *password = ap_password ? ap_password : (strlen(wm->ap_password) > 0 ? wm->ap_password : NULL);
    if (password && strlen(password) >= 8)
    {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Ensure STA interface is disconnected before starting scan
    esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_start());

    // Start web server for configuration
    start_webserver();
    update_status(WIFI_STATUS_AP_MODE);

    ESP_LOGI(TAG, "AP mode started. SSID: %s", ssid);
    if (password && strlen(password) >= 8)
    {
        ESP_LOGI(TAG, "AP Password: %s", password);
    }
    else
    {
        ESP_LOGI(TAG, "AP is open (no password)");
    }
    ESP_LOGI(TAG, "Connect to WiFi network '%s' and go to http://192.168.4.1", ssid);

    // Only schedule WiFi scan if we're truly in config portal mode
    // and not auto-connecting to saved credentials
    ESP_LOGI(TAG, "Config portal started - starting WiFi scanning with new task-based approach");

    // Trigger initial scan after a short delay to let AP mode stabilize
    vTaskDelay(pdMS_TO_TICKS(2000)); // 2 second delay
    trigger_wifi_scan(wm);

    // Set up timeout timer if configured
    if (wm->config_portal_timeout > 0)
    {
        wm->timeout_timer = xTimerCreate("wm_timeout",
                                         pdMS_TO_TICKS(wm->config_portal_timeout * 1000),
                                         pdFALSE, wm, timeout_timer_callback);
        if (wm->timeout_timer)
        {
            xTimerStart(wm->timeout_timer, 0);
        }
    }

    // Wait for configuration or timeout
    while (!wm->portal_aborted && !wm->config_saved)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second instead of 100ms

        // Check for timeout
        if (wm->timeout_timer && !xTimerIsTimerActive(wm->timeout_timer))
        {
            ESP_LOGW(TAG, "Config portal timeout reached");
            break;
        }

        // Check if we got a successful connection through the web interface
        if (current_status == WIFI_STATUS_CONNECTED)
        {
            wm->config_saved = true;
            break;
        }
    }

    // Clean up timer
    if (wm->timeout_timer)
    {
        xTimerStop(wm->timeout_timer, 0);
        xTimerDelete(wm->timeout_timer, portMAX_DELAY);
        wm->timeout_timer = NULL;
    }

    if (wm->config_saved)
    {
        ESP_LOGI(TAG, "Configuration saved, attempting to connect");
        if (wm->save_callback)
        {
            wm->save_callback();
        }
        return true;
    }
    else
    {
        ESP_LOGW(TAG, "Config portal timeout or aborted");
        return false;
    }
}

/* ==========================================
 *          SETTER FUNCTIONS
 * ========================================== */

void wifi_manager_set_ap_callback(wifi_manager_t *wm, config_mode_callback_t callback)
{
    if (wm)
    {
        wm->ap_callback = callback;
    }
}

void wifi_manager_set_save_config_callback(wifi_manager_t *wm, save_config_callback_t callback)
{
    if (wm)
    {
        wm->save_callback = callback;
    }
}

void wifi_manager_set_config_portal_timeout(wifi_manager_t *wm, uint32_t timeout_seconds)
{
    if (wm)
    {
        wm->config_portal_timeout = timeout_seconds;
        if (wm->debug_output)
        {
            ESP_LOGI(TAG, "Config portal timeout set to %lu seconds", (unsigned long)timeout_seconds);
        }
    }
}

void wifi_manager_set_minimum_signal_quality(wifi_manager_t *wm, int quality)
{
    if (wm)
    {
        wm->minimum_signal_quality = MAX(0, MIN(100, quality));
        if (wm->debug_output)
        {
            ESP_LOGI(TAG, "Minimum signal quality set to %d%%", wm->minimum_signal_quality);
        }
    }
}

void wifi_manager_set_debug_output(wifi_manager_t *wm, bool debug)
{
    if (wm)
    {
        wm->debug_output = debug;
    }
}

/* ==========================================
 *          GETTER FUNCTIONS
 * ========================================== */

wifi_status_t wifi_manager_get_status(wifi_manager_t *wm)
{
    return wm ? wm->current_status : WIFI_STATUS_DISCONNECTED;
}

const char *wifi_manager_get_ip_address(wifi_manager_t *wm)
{
    if (wm && wm->current_status == WIFI_STATUS_CONNECTED)
    {
        return wm->ip_address;
    }
    return NULL;
}

const char *wifi_manager_get_config_portal_ssid(wifi_manager_t *wm)
{
    return wm ? wm->ap_ssid : NULL;
}

/* ==========================================
 *          UTILITY FUNCTIONS
 * ========================================== */

esp_err_t wifi_manager_erase_config(wifi_manager_t *wm)
{
    if (!wm)
        return ESP_ERR_INVALID_ARG;

    // Erase our custom WiFiManager configuration
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        return err;

    nvs_erase_all(nvs_handle);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    // Also erase ESP-IDF's default WiFi configuration storage
    nvs_handle_t wifi_nvs_handle;
    err = nvs_open("nvs.net80211", NVS_READWRITE, &wifi_nvs_handle);
    if (err == ESP_OK)
    {
        nvs_erase_all(wifi_nvs_handle);
        nvs_commit(wifi_nvs_handle);
        nvs_close(wifi_nvs_handle);
    }

    // Clear any current WiFi configuration in memory
    wifi_config_t wifi_config = {0};
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    if (wm->debug_output)
    {
        ESP_LOGI(TAG, "WiFi configuration erased (both custom and ESP-IDF)");
    }

    return ESP_OK;
}

/* ==========================================
 *          LEGACY API FUNCTIONS
 * ========================================== */

esp_err_t wifi_manager_init(wifi_event_callback_t callback)
{
    user_callback = callback;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create network interfaces
    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    ESP_LOGI(TAG, "WiFi Manager initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    char ssid[33] = {0};
    char password[65] = {0};

    // Try to load saved credentials
    if (load_wifi_credentials(ssid, password) == ESP_OK && strlen(ssid) > 0)
    {
        ESP_LOGI(TAG, "Found saved WiFi credentials, attempting to connect to: %s", ssid);

        // Set to STA mode and start WiFi
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

        // Use the scan task instead of direct scan to avoid conflicts
        wifi_manager_t *wm = g_wm;
        if (!wm)
        {
            ESP_LOGE(TAG, "WiFi Manager not initialized");
            return ESP_ERR_INVALID_STATE;
        }

        ESP_LOGI(TAG, "Using scan task to find networks...");

        // Reset scan state
        wm->scan_completed = false;
        wm->scanned_count = 0;

        // Trigger scan using the scan task
        trigger_wifi_scan(wm);

        // Wait for scan completion with timeout
        int scan_wait_ms = 0;
        const int scan_timeout_ms = 15000; // 15 second timeout for scan
        const int poll_interval_ms = 100;

        while (!wm->scan_completed && scan_wait_ms < scan_timeout_ms)
        {
            vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
            scan_wait_ms += poll_interval_ms;
        }

        if (!wm->scan_completed)
        {
            ESP_LOGW(TAG, "Scan timeout after %d ms", scan_timeout_ms);
        }
        else
        {
            ESP_LOGI(TAG, "Scan completed via scan task. Found %d networks", wm->scanned_count);
        }

        // Look for the saved SSID in scan results
        int strongest_index = -1;
        int8_t strongest_rssi = -128;

        if (wm->scanned_count > 0)
        {
            for (int i = 0; i < wm->scanned_count && i < MAX_SCANNED_NETWORKS; i++)
            {
                ESP_LOGI(TAG, "Scan result %d: SSID='%s', RSSI=%d", i, wm->scanned_networks[i].ssid, wm->scanned_networks[i].rssi);
                if (strcmp(wm->scanned_networks[i].ssid, ssid) == 0)
                {
                    if (wm->scanned_networks[i].rssi > strongest_rssi)
                    {
                        strongest_rssi = wm->scanned_networks[i].rssi;
                        strongest_index = i;
                    }
                }
            }
        }

        // Configure STA mode with strongest AP
        wifi_config_t wifi_config = {0};
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

        if (strongest_index >= 0)
        {
            ESP_LOGI(TAG, "Found saved network '%s' with RSSI: %d dBm", ssid, strongest_rssi);
            ESP_LOGI(TAG, "Connecting to strongest AP: %s (RSSI: %d dBm)",
                     ssid, wm->scanned_networks[strongest_index].rssi);
        }
        else
        {
            ESP_LOGW(TAG, "No AP found with SSID %s, attempting connection anyway", ssid);
        }

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        update_status(WIFI_STATUS_CONNECTING);

        // Start connection attempt
        esp_err_t connect_result = esp_wifi_connect();
        if (connect_result == ESP_OK)
        {
            ESP_LOGI(TAG, "WiFi connection initiated successfully");
            return ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initiate WiFi connection: %s", esp_err_to_name(connect_result));
            return connect_result;
        }
    }
    else
    {
        ESP_LOGI(TAG, "No saved WiFi credentials, starting AP mode for setup");

        // Configure AP mode
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = WIFI_MANAGER_AP_SSID,
                .password = WIFI_MANAGER_AP_PASS,
                .ssid_len = strlen(WIFI_MANAGER_AP_SSID),
                .channel = 1,
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK},
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        // Start web server for configuration
        start_webserver();
        update_status(WIFI_STATUS_AP_MODE);

        ESP_LOGI(TAG, "AP mode started. SSID: %s, Password: %s", WIFI_MANAGER_AP_SSID, WIFI_MANAGER_AP_PASS);
        ESP_LOGI(TAG, "Connect to this network and go to http://192.168.4.1 to configure WiFi");
    }

    return ESP_OK;
}

wifi_status_t wifi_manager_get_current_status(void)
{
    return current_status;
}

const char *wifi_manager_get_current_ip(void)
{
    return (current_status == WIFI_STATUS_CONNECTED) ? ip_address : NULL;
}

esp_err_t wifi_manager_reset_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        return err;

    nvs_erase_all(nvs_handle);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials cleared");
    return ESP_OK;
}

esp_err_t wifi_manager_stop(void)
{
    stop_webserver();
    esp_wifi_stop();
    update_status(WIFI_STATUS_DISCONNECTED);
    return ESP_OK;
}

/* ==========================================
 *      CONFIGURATION MANAGEMENT API
 * ========================================== */

esp_err_t wifi_manager_add_parameter(wifi_manager_t *wm, const char *key, const char *label,
                                     const char *default_value, bool required, const char *placeholder)
{
    return add_config_parameter(wm, key, label, CONFIG_TYPE_STRING, default_value, required, placeholder);
}

esp_err_t wifi_manager_set_parameter(wifi_manager_t *wm, const char *key, const char *value)
{
    return set_config_parameter(wm, key, value);
}

esp_err_t wifi_manager_get_parameter(wifi_manager_t *wm, const char *key, char *value, size_t value_len)
{
    return get_config_parameter(wm, key, value, value_len);
}

esp_err_t wifi_manager_get_parameter_int(wifi_manager_t *wm, const char *key, int *value)
{
    if (!wm || !key || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char str_value[32];
    esp_err_t err = get_config_parameter(wm, key, str_value, sizeof(str_value));
    if (err == ESP_OK)
    {
        *value = atoi(str_value);
    }
    return err;
}

esp_err_t wifi_manager_get_parameter_bool(wifi_manager_t *wm, const char *key, bool *value)
{
    if (!wm || !key || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char str_value[16];
    esp_err_t err = get_config_parameter(wm, key, str_value, sizeof(str_value));
    if (err == ESP_OK)
    {
        *value = (strcmp(str_value, "true") == 0 || strcmp(str_value, "1") == 0);
    }
    return err;
}

esp_err_t wifi_manager_save_config(wifi_manager_t *wm)
{
    return save_config_parameters(wm);
}

esp_err_t wifi_manager_load_config(wifi_manager_t *wm)
{
    return load_config_parameters(wm);
}

esp_err_t wifi_manager_reset_config(wifi_manager_t *wm)
{
    if (!wm)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Reset all parameters to their default values
    for (int i = 0; i < wm->config_param_count; i++)
    {
        config_param_t *param = &wm->config_params[i];
        strncpy(param->value, param->default_value, sizeof(param->value) - 1);
        param->value[sizeof(param->value) - 1] = '\0';
    }

    ESP_LOGI(TAG, "Configuration parameters reset to defaults");
    return ESP_OK;
}