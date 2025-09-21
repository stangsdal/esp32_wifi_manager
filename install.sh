#!/bin/bash

# ESP32 WiFi Manager Component Installation Script
# This script helps integrate the WiFi Manager component into your ESP-IDF project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}ESP32 WiFi Manager Component Installation${NC}"
echo "=========================================="

# Check if we're in an ESP-IDF project
if [ ! -f "CMakeLists.txt" ] || [ ! -d "main" ]; then
    echo -e "${RED}Error: This doesn't appear to be an ESP-IDF project directory${NC}"
    echo "Please run this script from your ESP-IDF project root directory"
    exit 1
fi

echo -e "${GREEN}✓ ESP-IDF project detected${NC}"

# Create components directory if it doesn't exist
if [ ! -d "components" ]; then
    echo -e "${YELLOW}Creating components directory...${NC}"
    mkdir -p components
fi

# Check installation method preference
echo ""
echo "Choose installation method:"
echo "1) Git submodule (recommended for development)"
echo "2) Copy component files"
echo "3) ESP-IDF Component Manager (if available)"
read -p "Enter choice (1-3): " choice

case $choice in
    1)
        echo -e "${YELLOW}Installing as git submodule...${NC}"
        if [ -d "components/wifi_manager" ]; then
            echo -e "${YELLOW}WiFi Manager component already exists, removing...${NC}"
            rm -rf components/wifi_manager
        fi
        git submodule add https://github.com/pstangsdal/esp32_wifi_manager.git components/wifi_manager
        git submodule update --init --recursive
        echo -e "${GREEN}✓ Component installed as git submodule${NC}"
        ;;
    2)
        echo -e "${YELLOW}This option requires manual download of the component${NC}"
        echo "Please download the component and copy it to components/wifi_manager/"
        exit 1
        ;;
    3)
        echo -e "${YELLOW}Adding component via ESP-IDF Component Manager...${NC}"
        if command -v idf.py &> /dev/null; then
            idf.py add-dependency "pstangsdal/esp32_wifi_manager"
            echo -e "${GREEN}✓ Component added via Component Manager${NC}"
        else
            echo -e "${RED}Error: idf.py not found. Please set up ESP-IDF environment first${NC}"
            exit 1
        fi
        ;;
    *)
        echo -e "${RED}Invalid choice${NC}"
        exit 1
        ;;
esac

# Create basic example if requested
echo ""
read -p "Create a basic example in main/main.c? (y/n): " create_example

if [ "$create_example" = "y" ] || [ "$create_example" = "Y" ]; then
    if [ -f "main/main.c" ]; then
        echo -e "${YELLOW}Backing up existing main.c to main.c.backup${NC}"
        cp main/main.c main/main.c.backup
    fi
    
    cat > main/main.c << 'EOF'
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"

static const char *TAG = "MAIN";

void save_config_callback(void)
{
    ESP_LOGI(TAG, "Configuration saved!");
}

void config_mode_callback(wifi_manager_t *wm)
{
    ESP_LOGI(TAG, "Config portal started - connect to ESP32 AP and visit http://192.168.4.1");
}

void app_main(void)
{
    ESP_LOGI(TAG, "WiFi Manager Example Starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create WiFi Manager
    wifi_manager_t *wm = wifi_manager_create();
    
    // Set callbacks
    wifi_manager_set_save_config_callback(wm, save_config_callback);
    wifi_manager_set_ap_callback(wm, config_mode_callback);
    
    // Add custom parameters
    wifi_manager_add_parameter(wm, "device_name", "Device Name", "ESP32-Device", 32, NULL, NULL);
    
    // Auto-connect
    if (wifi_manager_auto_connect(wm, "ESP32-Setup", NULL)) {
        ESP_LOGI(TAG, "Connected to WiFi successfully!");
    } else {
        ESP_LOGI(TAG, "Failed to connect - config portal should be running");
    }

    // Main loop
    while (1) {
        ESP_LOGI(TAG, "Application running...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
EOF

    echo -e "${GREEN}✓ Basic example created in main/main.c${NC}"
fi

# Update main CMakeLists.txt to include wifi_manager dependency
if [ -f "main/CMakeLists.txt" ]; then
    if ! grep -q "wifi_manager" main/CMakeLists.txt; then
        echo -e "${YELLOW}Updating main/CMakeLists.txt to include wifi_manager dependency...${NC}"
        sed -i.bak 's/REQUIRES/REQUIRES wifi_manager/' main/CMakeLists.txt
        echo -e "${GREEN}✓ CMakeLists.txt updated${NC}"
    fi
fi

echo ""
echo -e "${GREEN}🎉 Installation Complete!${NC}"
echo ""
echo "Next steps:"
echo "1. Build your project: ${BLUE}idf.py build${NC}"
echo "2. Flash to device: ${BLUE}idf.py flash monitor${NC}"
echo "3. Connect to 'ESP32-Setup' WiFi and visit http://192.168.4.1"
echo ""
echo "For more information, see:"
echo "- README.md in components/wifi_manager/"
echo "- Examples in components/wifi_manager/examples/"
echo ""
echo -e "${YELLOW}Happy coding! 🚀${NC}"