# WiFiManager Component for ESP-IDF
# Provides tzapu/WiFiManager compatible API for ESP32 projects
# Author: Peter Stangsdal
# License: MIT
# Version: 2.0.1
# Date: 2025-12-25

idf_component_register(
    SRCS 
        "src/wifi_manager_core.c"
        "src/wifi_manager_scan.c"
        "src/wifi_manager_web.c"
        "src/wifi_manager_storage.c"
        "src/wifi_manager_config.c"
        "src/wifi_manager_api.c"
    INCLUDE_DIRS "." "src"
    REQUIRES esp_wifi nvs_flash esp_netif esp_event esp_http_server lwip freertos espressif__cjson
    EMBED_FILES 
        "web/setup.html"
        "web/style.css"
        "web/script.js"
        "web/success.html"
        "web/config.html"
)