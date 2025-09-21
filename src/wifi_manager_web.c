/**
 * @file wifi_manager_web.c
 * @brief Web server and HTTP request handlers
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"

/**
 * @brief Handler for main setup page - smart routing based on WiFi status
 */
esp_err_t setup_page_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Main page requested - checking WiFi status");

    if (!g_wm)
    {
        ESP_LOGE(TAG, "WiFi Manager not initialized");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFi Manager not initialized");
        return ESP_FAIL;
    }

    // Check current WiFi status
    wifi_status_t status = current_status;

    // If connected, show configuration page instead of setup page
    if (status == WIFI_STATUS_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi connected - serving configuration page");
        return config_html_handler(req);
    }
    else
    {
        ESP_LOGI(TAG, "WiFi not connected - serving setup page with scan");
        // Trigger a fresh scan when someone accesses the portal and not connected
        trigger_wifi_scan(g_wm);
        return setup_html_handler(req);
    }
}

/**
 * @brief Handler for serving embedded HTML
 */
esp_err_t setup_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Setup HTML requested");

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    size_t html_size = setup_html_end - setup_html_start;
    return httpd_resp_send(req, (const char *)setup_html_start, html_size);
}

/**
 * @brief Handler for serving embedded CSS
 */
esp_err_t style_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Style CSS requested");

    httpd_resp_set_type(req, "text/css");

    size_t css_size = style_css_end - style_css_start;
    return httpd_resp_send(req, (const char *)style_css_start, css_size);
}

/**
 * @brief Handler for serving embedded JavaScript
 */
esp_err_t script_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Script JS requested");

    httpd_resp_set_type(req, "application/javascript");

    size_t js_size = script_js_end - script_js_start;
    return httpd_resp_send(req, (const char *)script_js_start, js_size);
}

/**
 * @brief Handler for serving embedded success page
 */
esp_err_t success_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Success HTML requested");

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    size_t html_size = success_html_end - success_html_start;
    return httpd_resp_send(req, (const char *)success_html_start, html_size);
}

/**
 * @brief Handler for serving embedded configuration page
 */
esp_err_t config_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Configuration HTML requested");

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    size_t html_size = config_html_end - config_html_start;
    return httpd_resp_send(req, (const char *)config_html_start, html_size);
}

/**
 * @brief Handler for WiFi connection requests
 */
esp_err_t connect_handler(httpd_req_t *req)
{
    char buf[1000];
    char ssid[33] = {0};
    char password[65] = {0};
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    buf[req->content_len] = '\0';

    // Parse form data (simple parsing for ssid=...&password=...)
    char *ssid_start = strstr(buf, "ssid=");
    char *password_start = strstr(buf, "password=");

    if (ssid_start)
    {
        ssid_start += 5; // Skip "ssid="
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end)
        {
            strncpy(ssid, ssid_start, MIN(ssid_end - ssid_start, sizeof(ssid) - 1));
        }
        else
        {
            strncpy(ssid, ssid_start, sizeof(ssid) - 1);
        }
    }

    if (password_start)
    {
        password_start += 9; // Skip "password="
        char *password_end = strchr(password_start, '&');
        if (password_end)
        {
            strncpy(password, password_start, MIN(password_end - password_start, sizeof(password) - 1));
        }
        else
        {
            strncpy(password, password_start, sizeof(password) - 1);
        }
    }

    ESP_LOGI(TAG, "Received WiFi credentials - SSID: %s", ssid);

    // Save credentials and try to connect
    save_wifi_credentials(ssid, password);

    // Send success page
    success_html_handler(req);

    // Configure and start STA mode
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();

    return ESP_OK;
}

/**
 * @brief HTTP handler to return available WiFi networks as JSON
 */
esp_err_t wifi_list_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi list requested");

    if (!g_wm)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFiManager not initialized");
        return ESP_FAIL;
    }

    // Set content type to JSON
    httpd_resp_set_type(req, "application/json");

    // Check if already connected - return current connection info instead of scan
    if (current_status == WIFI_STATUS_CONNECTED)
    {
        ESP_LOGI(TAG, "Already connected - returning current connection info");

        // Get current WiFi info
        wifi_ap_record_t ap_info;
        esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

        // Get IP address
        char ip_str[16] = "Unknown";
        esp_netif_ip_info_t ip_info;
        if (sta_netif && esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK)
        {
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
        }

        if (ret == ESP_OK)
        {
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"connected\":true,\"current_network\":\"%s\",\"signal\":%d,\"ip\":\"%s\",\"networks\":[]}",
                     (char *)ap_info.ssid, ap_info.rssi, ip_str);
            return httpd_resp_send(req, response, -1);
        }
        else
        {
            // Fallback if we can't get current AP info
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"connected\":true,\"current_network\":\"Connected\",\"ip\":\"%s\",\"networks\":[]}", ip_str);
            return httpd_resp_send(req, response, -1);
        }
    }

    ESP_LOGI(TAG, "Not connected - returning scan results: scan_completed: %s, count: %d",
             g_wm->scan_completed ? "true" : "false", g_wm->scanned_count);

    // Allocate buffer for JSON response
    char *json_response = malloc(4096);
    if (!json_response)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int offset = snprintf(json_response, 4096, "{\"connected\":false,\"networks\":[");

    // Only process networks if scan is completed
    if (g_wm->scan_completed && g_wm->scanned_count > 0)
    {
        // Create a unique network list (strongest signal per SSID)
        scanned_network_t unique_networks[MAX_SCANNED_NETWORKS];
        int unique_count = 0;
        int output_count = 0;

        // First pass: collect unique SSIDs with strongest signal
        for (int i = 0; i < g_wm->scanned_count; i++)
        {
            const char *current_ssid = g_wm->scanned_networks[i].ssid;

            // Skip hidden networks and empty SSIDs
            if (strlen(current_ssid) == 0 || g_wm->scanned_networks[i].is_hidden)
            {
                continue;
            }

            // Check if this SSID already exists in unique list
            int existing_index = -1;
            for (int j = 0; j < unique_count; j++)
            {
                if (strcmp(unique_networks[j].ssid, current_ssid) == 0)
                {
                    existing_index = j;
                    break;
                }
            }

            if (existing_index >= 0)
            {
                // SSID exists, keep the one with stronger signal
                if (g_wm->scanned_networks[i].rssi > unique_networks[existing_index].rssi)
                {
                    unique_networks[existing_index] = g_wm->scanned_networks[i];
                }
            }
            else if (unique_count < MAX_SCANNED_NETWORKS)
            {
                // New SSID, add to unique list
                unique_networks[unique_count] = g_wm->scanned_networks[i];
                unique_count++;
            }
        }

        // Second pass: sort by signal strength (strongest first)
        for (int i = 0; i < unique_count - 1; i++)
        {
            for (int j = i + 1; j < unique_count; j++)
            {
                if (unique_networks[j].rssi > unique_networks[i].rssi)
                {
                    scanned_network_t temp = unique_networks[i];
                    unique_networks[i] = unique_networks[j];
                    unique_networks[j] = temp;
                }
            }
        }

        // Third pass: generate JSON for unique networks
        for (int i = 0; i < unique_count; i++)
        {
            int strongest_index = i;

            // Determine authentication type string
            const char *auth_str;
            switch (unique_networks[strongest_index].authmode)
            {
            case WIFI_AUTH_OPEN:
                auth_str = "Open";
                break;
            case WIFI_AUTH_WEP:
                auth_str = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                auth_str = "WPA";
                break;
            case WIFI_AUTH_WPA2_PSK:
                auth_str = "WPA2";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                auth_str = "WPA/WPA2";
                break;
            case WIFI_AUTH_WPA3_PSK:
                auth_str = "WPA3";
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                auth_str = "WPA2/WPA3";
                break;
            default:
                auth_str = "Open";
                break;
            }

            // Calculate signal quality percentage (RSSI to percentage)
            int quality = 0;
            if (unique_networks[strongest_index].rssi >= -50)
            {
                quality = 100;
            }
            else if (unique_networks[strongest_index].rssi >= -60)
            {
                quality = 90;
            }
            else if (unique_networks[strongest_index].rssi >= -70)
            {
                quality = 70;
            }
            else if (unique_networks[strongest_index].rssi >= -80)
            {
                quality = 50;
            }
            else if (unique_networks[strongest_index].rssi >= -90)
            {
                quality = 25;
            }
            else
            {
                quality = 10;
            }

            offset += snprintf(json_response + offset, 4096 - offset,
                               "%s{\"ssid\":\"%s\",\"rssi\":%d,\"quality\":%d,\"auth\":\"%s\",\"secure\":%s}",
                               (output_count > 0) ? "," : "",
                               unique_networks[strongest_index].ssid,
                               unique_networks[strongest_index].rssi,
                               quality,
                               auth_str,
                               (unique_networks[strongest_index].authmode == WIFI_AUTH_OPEN) ? "false" : "true");
            output_count++;
        }
    }

    offset += snprintf(json_response + offset, 4096 - offset,
                       "],\"scan_completed\":%s,\"count\":%d}",
                       g_wm->scan_completed ? "true" : "false",
                       g_wm->scanned_count);

    ESP_LOGI(TAG, "Sending WiFi JSON response (%d bytes)", offset);
    httpd_resp_send(req, json_response, offset);

    free(json_response);
    return ESP_OK;
}

/**
 * @brief Start the HTTP web server
 */
esp_err_t start_webserver(void)
{
    if (!g_wm)
    {
        ESP_LOGE(TAG, "WiFiManager not initialized");
        return ESP_FAIL;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16; // Increase from default 8 to accommodate all our handlers

    if (httpd_start(&g_wm->server, &config) == ESP_OK)
    {
        httpd_uri_t setup_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = setup_page_handler,
        };
        httpd_register_uri_handler(g_wm->server, &setup_uri);

        httpd_uri_t connect_uri = {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = connect_handler,
        };
        httpd_register_uri_handler(g_wm->server, &connect_uri);

        httpd_uri_t wifi_list_uri = {
            .uri = "/wifi",
            .method = HTTP_GET,
            .handler = wifi_list_handler,
        };
        httpd_register_uri_handler(g_wm->server, &wifi_list_uri);

        // Register static file handlers
        httpd_uri_t style_css_uri = {
            .uri = "/style.css",
            .method = HTTP_GET,
            .handler = style_css_handler,
        };
        httpd_register_uri_handler(g_wm->server, &style_css_uri);

        httpd_uri_t script_js_uri = {
            .uri = "/script.js",
            .method = HTTP_GET,
            .handler = script_js_handler,
        };
        httpd_register_uri_handler(g_wm->server, &script_js_uri);

        // Register configuration page handler
        httpd_uri_t config_html_uri = {
            .uri = "/config.html",
            .method = HTTP_GET,
            .handler = config_html_handler,
        };
        httpd_register_uri_handler(g_wm->server, &config_html_uri);

        // Register configuration handlers
        httpd_uri_t config_uri = {
            .uri = "/config",
            .method = HTTP_GET,
            .handler = config_handler,
        };
        httpd_register_uri_handler(g_wm->server, &config_uri);

        httpd_uri_t config_save_uri = {
            .uri = "/config/save",
            .method = HTTP_POST,
            .handler = config_save_handler,
        };
        httpd_register_uri_handler(g_wm->server, &config_save_uri);

        // Register device management handlers
        httpd_uri_t restart_uri = {
            .uri = "/restart",
            .method = HTTP_POST,
            .handler = restart_handler,
        };
        httpd_register_uri_handler(g_wm->server, &restart_uri);

        httpd_uri_t reset_uri = {
            .uri = "/reset",
            .method = HTTP_POST,
            .handler = reset_handler,
        };
        httpd_register_uri_handler(g_wm->server, &reset_uri);

        httpd_uri_t wifi_reset_uri = {
            .uri = "/wifi-reset",
            .method = HTTP_POST,
            .handler = wifi_reset_handler,
        };
        httpd_register_uri_handler(g_wm->server, &wifi_reset_uri);

        ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start web server");
        return ESP_FAIL;
    }
}

/**
 * @brief Stop the HTTP web server
 */
void stop_webserver(void)
{
    if (g_wm && g_wm->server)
    {
        ESP_LOGI(TAG, "Stopping web server");
        httpd_stop(g_wm->server);
        g_wm->server = NULL;
    }
}

/**
 * @brief Handler for configuration parameters JSON API
 */
esp_err_t config_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Configuration parameters requested");

    if (!g_wm)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "WiFiManager not initialized");
        return ESP_FAIL;
    }

    // Set content type to JSON
    httpd_resp_set_type(req, "application/json");

    // Allocate buffer for JSON response
    char *json_response = malloc(4096);
    if (!json_response)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int offset = snprintf(json_response, 4096, "{\"parameters\":[");

    // Add all configuration parameters
    for (int i = 0; i < g_wm->config_param_count; i++)
    {
        config_param_t *param = &g_wm->config_params[i];

        const char *type_str;
        switch (param->type)
        {
        case CONFIG_TYPE_STRING:
            type_str = "string";
            break;
        case CONFIG_TYPE_INT:
            type_str = "number";
            break;
        case CONFIG_TYPE_FLOAT:
            type_str = "number";
            break;
        case CONFIG_TYPE_BOOL:
            type_str = "checkbox";
            break;
        default:
            type_str = "text";
            break;
        }

        offset += snprintf(json_response + offset, 4096 - offset,
                           "%s{\"key\":\"%s\",\"label\":\"%s\",\"type\":\"%s\",\"value\":\"%s\",\"placeholder\":\"%s\",\"required\":%s}",
                           (i > 0) ? "," : "",
                           param->key,
                           param->label,
                           type_str,
                           param->value,
                           param->placeholder,
                           param->required ? "true" : "false");
    }

    offset += snprintf(json_response + offset, 4096 - offset, "]}");

    ESP_LOGI(TAG, "Sending config JSON response (%d bytes)", offset);
    httpd_resp_send(req, json_response, offset);

    free(json_response);
    return ESP_OK;
}

/**
 * @brief Handler for saving configuration parameters
 */
esp_err_t config_save_handler(httpd_req_t *req)
{
    char buf[2048];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request too large");
        return ESP_FAIL;
    }

    // Read the request body
    int total_read = 0;
    while (remaining > 0 && total_read < sizeof(buf) - 1)
    {
        if ((ret = httpd_req_recv(req, buf + total_read, remaining)) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
        total_read += ret;
    }
    buf[total_read] = '\0';

    ESP_LOGI(TAG, "Received config data: %s", buf);

    // Parse form data and update configuration parameters
    char *token = strtok(buf, "&");
    bool config_updated = false;

    while (token != NULL)
    {
        char *equals = strchr(token, '=');
        if (equals)
        {
            *equals = '\0';
            char *key = token;
            char *value = equals + 1;

            // URL decode the value (basic implementation)
            char decoded_value[256];
            int decoded_len = 0;
            for (int i = 0; value[i] && decoded_len < sizeof(decoded_value) - 1; i++)
            {
                if (value[i] == '+')
                {
                    decoded_value[decoded_len++] = ' ';
                }
                else if (value[i] == '%' && value[i + 1] && value[i + 2])
                {
                    // Simple hex decode
                    char hex[3] = {value[i + 1], value[i + 2], '\0'};
                    decoded_value[decoded_len++] = (char)strtol(hex, NULL, 16);
                    i += 2;
                }
                else
                {
                    decoded_value[decoded_len++] = value[i];
                }
            }
            decoded_value[decoded_len] = '\0';

            // Update the configuration parameter
            if (set_config_parameter(g_wm, key, decoded_value) == ESP_OK)
            {
                config_updated = true;
            }
        }
        token = strtok(NULL, "&");
    }

    if (config_updated)
    {
        // Save configuration to NVS
        esp_err_t err = save_config_parameters(g_wm);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Configuration saved successfully");

            // Send success response
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Configuration saved\"}", -1);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(err));
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGW(TAG, "No configuration parameters were updated");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"status\":\"warning\",\"message\":\"No changes detected\"}", -1);
    }

    return ESP_OK;
}

/**
 * @brief Handler for device restart requests
 */
esp_err_t restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Device restart requested");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Device restarting...\"}", -1);

    // Restart after a brief delay to allow response to be sent
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}

/**
 * @brief Handler for factory reset requests
 */
esp_err_t reset_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Factory reset requested");

    httpd_resp_set_type(req, "application/json");

    // Erase WiFi configuration
    esp_err_t wifi_err = wifi_manager_erase_config(g_wm);

    // Reset configuration parameters to defaults
    esp_err_t config_err = reset_config_parameters(g_wm);

    if (wifi_err == ESP_OK && config_err == ESP_OK)
    {
        httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Settings reset. Device will restart.\"}", -1);

        // Restart after a brief delay
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Failed to reset settings - WiFi: %s, Config: %s",
                 esp_err_to_name(wifi_err), esp_err_to_name(config_err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to reset settings");
    }

    return ESP_OK;
}

/**
 * @brief Handler for WiFi reset requests (keep device config, only reset WiFi)
 */
esp_err_t wifi_reset_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi reset requested");

    httpd_resp_set_type(req, "application/json");

    // Only erase WiFi configuration, keep device configuration
    esp_err_t wifi_err = wifi_manager_erase_config(g_wm);

    if (wifi_err == ESP_OK)
    {
        httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"WiFi settings reset. Returning to setup mode.\"}", -1);

        // Disconnect from current WiFi and restart in AP mode after a brief delay
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Stop STA mode and start AP mode
        esp_wifi_disconnect();
        update_status(WIFI_STATUS_DISCONNECTED);

        // The device will automatically start the config portal due to no saved credentials
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Failed to reset WiFi settings: %s", esp_err_to_name(wifi_err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to reset WiFi settings");
    }

    return ESP_OK;
}