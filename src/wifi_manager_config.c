/**
 * @file wifi_manager_config.c
 * @brief Configuration parameter management and storage
 * @version 2.0.0
 * @date 2025-09-21
 * @author Peter Stangsdal
 */

#include "wifi_manager_private.h"
#include "cJSON.h"

/**
 * @brief Initialize default configuration parameters (MQTT example)
 */
void init_default_config_parameters(wifi_manager_t *wm)
{
    if (!wm)
        return;

    wm->config_param_count = 0;
    wm->config_portal_enabled = true;

    // Add default MQTT configuration parameters
    add_config_parameter(wm, "mqtt_broker", "MQTT Broker", CONFIG_TYPE_STRING,
                         "broker.mqtt.cool", true, "mqtt.example.com");

    add_config_parameter(wm, "mqtt_port", "MQTT Port", CONFIG_TYPE_INT,
                         "1883", true, "1883");

    add_config_parameter(wm, "mqtt_username", "MQTT Username", CONFIG_TYPE_STRING,
                         "", false, "username");

    add_config_parameter(wm, "mqtt_password", "MQTT Password", CONFIG_TYPE_STRING,
                         "", false, "password");

    add_config_parameter(wm, "mqtt_topic", "MQTT Topic Prefix", CONFIG_TYPE_STRING,
                         "esp32/device", true, "esp32/device");

    add_config_parameter(wm, "device_name", "Device Name", CONFIG_TYPE_STRING,
                         "ESP32-CYD", true, "My ESP32 Device");

    add_config_parameter(wm, "update_interval", "Update Interval (seconds)", CONFIG_TYPE_INT,
                         "30", true, "30");

    add_config_parameter(wm, "enable_debug", "Enable Debug Logging", CONFIG_TYPE_BOOL,
                         "false", false, "");
}

/**
 * @brief Add a configuration parameter
 */
esp_err_t add_config_parameter(wifi_manager_t *wm, const char *key, const char *label,
                               config_param_type_t type, const char *default_value,
                               bool required, const char *placeholder)
{
    if (!wm || !key || !label || wm->config_param_count >= MAX_CONFIG_PARAMS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    config_param_t *param = &wm->config_params[wm->config_param_count];

    // Set parameter properties
    strncpy(param->key, key, sizeof(param->key) - 1);
    param->key[sizeof(param->key) - 1] = '\0';

    strncpy(param->label, label, sizeof(param->label) - 1);
    param->label[sizeof(param->label) - 1] = '\0';

    param->type = type;
    param->required = required;

    // Set default value
    if (default_value)
    {
        strncpy(param->default_value, default_value, sizeof(param->default_value) - 1);
        param->default_value[sizeof(param->default_value) - 1] = '\0';
        strncpy(param->value, default_value, sizeof(param->value) - 1);
        param->value[sizeof(param->value) - 1] = '\0';
    }
    else
    {
        param->default_value[0] = '\0';
        param->value[0] = '\0';
    }

    // Set placeholder
    if (placeholder)
    {
        strncpy(param->placeholder, placeholder, sizeof(param->placeholder) - 1);
        param->placeholder[sizeof(param->placeholder) - 1] = '\0';
    }
    else
    {
        param->placeholder[0] = '\0';
    }

    // Set validation based on type
    switch (type)
    {
    case CONFIG_TYPE_STRING:
        param->min_length = required ? 1 : 0;
        param->max_length = MAX_CONFIG_STRING_LEN - 1;
        break;
    case CONFIG_TYPE_INT:
        param->min_length = 1;
        param->max_length = 10;
        strncpy(param->validation_pattern, "^-?[0-9]+$", sizeof(param->validation_pattern) - 1);
        break;
    case CONFIG_TYPE_BOOL:
        param->min_length = 0;
        param->max_length = 5;
        break;
    case CONFIG_TYPE_FLOAT:
        param->min_length = 1;
        param->max_length = 15;
        strncpy(param->validation_pattern, "^-?[0-9]+(\\.[0-9]+)?$", sizeof(param->validation_pattern) - 1);
        break;
    }

    wm->config_param_count++;

    ESP_LOGI(TAG, "Added config parameter: %s = %s", key, param->value);
    return ESP_OK;
}

/**
 * @brief Set a configuration parameter value
 */
esp_err_t set_config_parameter(wifi_manager_t *wm, const char *key, const char *value)
{
    if (!wm || !key)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Find the parameter
    for (int i = 0; i < wm->config_param_count; i++)
    {
        if (strcmp(wm->config_params[i].key, key) == 0)
        {
            config_param_t *param = &wm->config_params[i];

            // Validate value based on type
            if (value && strlen(value) > 0)
            {
                // Type-specific validation
                switch (param->type)
                {
                case CONFIG_TYPE_INT:
                {
                    char *endptr;
                    strtol(value, &endptr, 10);
                    if (*endptr != '\0')
                    {
                        ESP_LOGW(TAG, "Invalid integer value for %s: %s", key, value);
                        return ESP_ERR_INVALID_ARG;
                    }
                    break;
                }
                case CONFIG_TYPE_FLOAT:
                {
                    char *endptr;
                    strtof(value, &endptr);
                    if (*endptr != '\0')
                    {
                        ESP_LOGW(TAG, "Invalid float value for %s: %s", key, value);
                        return ESP_ERR_INVALID_ARG;
                    }
                    break;
                }
                case CONFIG_TYPE_BOOL:
                {
                    if (strcmp(value, "true") != 0 && strcmp(value, "false") != 0 &&
                        strcmp(value, "1") != 0 && strcmp(value, "0") != 0)
                    {
                        ESP_LOGW(TAG, "Invalid boolean value for %s: %s", key, value);
                        return ESP_ERR_INVALID_ARG;
                    }
                    break;
                }
                case CONFIG_TYPE_STRING:
                    // String validation (length check)
                    if (strlen(value) > (size_t)param->max_length)
                    {
                        ESP_LOGW(TAG, "Value too long for %s: %s", key, value);
                        return ESP_ERR_INVALID_ARG;
                    }
                    break;
                }

                strncpy(param->value, value, sizeof(param->value) - 1);
                param->value[sizeof(param->value) - 1] = '\0';
            }
            else
            {
                // Empty value - check if required
                if (param->required)
                {
                    ESP_LOGW(TAG, "Required parameter %s cannot be empty", key);
                    return ESP_ERR_INVALID_ARG;
                }
                param->value[0] = '\0';
            }

            ESP_LOGI(TAG, "Set config parameter: %s = %s", key, param->value);
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "Configuration parameter not found: %s", key);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Get a configuration parameter value
 */
esp_err_t get_config_parameter(wifi_manager_t *wm, const char *key, char *value, size_t value_len)
{
    if (!wm || !key || !value || value_len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Find the parameter
    for (int i = 0; i < wm->config_param_count; i++)
    {
        if (strcmp(wm->config_params[i].key, key) == 0)
        {
            strncpy(value, wm->config_params[i].value, value_len - 1);
            value[value_len - 1] = '\0';
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "Configuration parameter not found: %s", key);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Save all configuration parameters to NVS
 */
esp_err_t save_config_parameters(wifi_manager_t *wm)
{
    if (!wm)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS handle for config: %s", esp_err_to_name(err));
        return err;
    }

    // Create JSON object for all parameters
    cJSON *json = cJSON_CreateObject();
    if (!json)
    {
        nvs_close(nvs_handle);
        ESP_LOGE(TAG, "Failed to create JSON object");
        return ESP_ERR_NO_MEM;
    }

    // Add all parameters to JSON
    for (int i = 0; i < wm->config_param_count; i++)
    {
        config_param_t *param = &wm->config_params[i];

        switch (param->type)
        {
        case CONFIG_TYPE_STRING:
            cJSON_AddStringToObject(json, param->key, param->value);
            break;
        case CONFIG_TYPE_INT:
        {
            int int_val = atoi(param->value);
            cJSON_AddNumberToObject(json, param->key, int_val);
            break;
        }
        case CONFIG_TYPE_FLOAT:
        {
            float float_val = atof(param->value);
            cJSON_AddNumberToObject(json, param->key, float_val);
            break;
        }
        case CONFIG_TYPE_BOOL:
        {
            bool bool_val = (strcmp(param->value, "true") == 0 || strcmp(param->value, "1") == 0);
            cJSON_AddBoolToObject(json, param->key, bool_val);
            break;
        }
        }
    }

    // Convert JSON to string
    char *json_string = cJSON_Print(json);
    if (!json_string)
    {
        cJSON_Delete(json);
        nvs_close(nvs_handle);
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return ESP_ERR_NO_MEM;
    }

    // Save to NVS
    err = nvs_set_str(nvs_handle, "config_json", json_string);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }

    // Cleanup
    free(json_string);
    cJSON_Delete(json);
    nvs_close(nvs_handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Configuration parameters saved to NVS");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to save configuration parameters: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Load all configuration parameters from NVS
 */
esp_err_t load_config_parameters(wifi_manager_t *wm)
{
    if (!wm)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS handle for config reading: %s", esp_err_to_name(err));
        return err;
    }

    // Get JSON string size
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "config_json", NULL, &required_size);
    if (err != ESP_OK)
    {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "No saved configuration found: %s", esp_err_to_name(err));
        return err;
    }

    // Allocate buffer and read JSON string
    char *json_string = malloc(required_size);
    if (!json_string)
    {
        nvs_close(nvs_handle);
        ESP_LOGE(TAG, "Failed to allocate memory for config JSON");
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, "config_json", json_string, &required_size);
    nvs_close(nvs_handle);

    if (err != ESP_OK)
    {
        free(json_string);
        ESP_LOGE(TAG, "Failed to read config JSON: %s", esp_err_to_name(err));
        return err;
    }

    // Parse JSON
    cJSON *json = cJSON_Parse(json_string);
    free(json_string);

    if (!json)
    {
        ESP_LOGE(TAG, "Failed to parse config JSON");
        return ESP_ERR_INVALID_ARG;
    }

    // Update parameter values from JSON
    for (int i = 0; i < wm->config_param_count; i++)
    {
        config_param_t *param = &wm->config_params[i];
        cJSON *json_item = cJSON_GetObjectItem(json, param->key);

        if (json_item)
        {
            switch (param->type)
            {
            case CONFIG_TYPE_STRING:
                if (cJSON_IsString(json_item))
                {
                    strncpy(param->value, json_item->valuestring, sizeof(param->value) - 1);
                    param->value[sizeof(param->value) - 1] = '\0';
                }
                break;
            case CONFIG_TYPE_INT:
                if (cJSON_IsNumber(json_item))
                {
                    snprintf(param->value, sizeof(param->value), "%d", (int)json_item->valuedouble);
                }
                break;
            case CONFIG_TYPE_FLOAT:
                if (cJSON_IsNumber(json_item))
                {
                    snprintf(param->value, sizeof(param->value), "%.2f", json_item->valuedouble);
                }
                break;
            case CONFIG_TYPE_BOOL:
                if (cJSON_IsBool(json_item))
                {
                    strncpy(param->value, cJSON_IsTrue(json_item) ? "true" : "false", sizeof(param->value) - 1);
                    param->value[sizeof(param->value) - 1] = '\0';
                }
                break;
            }
            ESP_LOGI(TAG, "Loaded config parameter: %s = %s", param->key, param->value);
        }
    }

    cJSON_Delete(json);
    ESP_LOGI(TAG, "Configuration parameters loaded from NVS");
    return ESP_OK;
}

/**
 * @brief Reset configuration parameters to defaults
 */
esp_err_t reset_config_parameters(wifi_manager_t *wm)
{
    if (!wm)
    {
        ESP_LOGE(TAG, "WiFi Manager is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Resetting configuration parameters to defaults");

    // Clear current parameters
    wm->config_param_count = 0;

    // Reinitialize with defaults
    init_default_config_parameters(wm);

    // Clear from NVS storage
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS for config reset: %s", esp_err_to_name(err));
        return err;
    }

    // Erase the config key from NVS
    err = nvs_erase_key(nvs_handle, "config_params");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Failed to erase config from NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit changes
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Configuration parameters reset to defaults successfully");
    return ESP_OK;
}