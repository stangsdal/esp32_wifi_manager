/**
 * @file wifi_manager_storage.c
 * @brief NVS storage operations for WiFi credentials
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"

/**
 * @brief Save WiFi credentials to NVS storage
 * @param ssid WiFi network name
 * @param password WiFi password
 * @return ESP_OK on success, error code on failure
 */
esp_err_t save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err == ESP_OK)
    {
        err = nvs_set_str(nvs_handle, "password", password);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi credentials saved to NVS");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to save WiFi credentials: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Load WiFi credentials from NVS storage
 * @param ssid Buffer to store WiFi network name (must be at least 33 bytes)
 * @param password Buffer to store WiFi password (must be at least 65 bytes)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t load_wifi_credentials(char *ssid, char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS handle for reading: %s", esp_err_to_name(err));
        return err;
    }

    size_t ssid_len = 32;
    size_t password_len = 64;

    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);
    if (err == ESP_OK)
    {
        err = nvs_get_str(nvs_handle, "password", password, &password_len);
    }

    nvs_close(nvs_handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi credentials loaded from NVS - SSID: %s", ssid);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to load WiFi credentials: %s", esp_err_to_name(err));
    }

    return err;
}