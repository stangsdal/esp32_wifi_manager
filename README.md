# ESP32 WiFi Manager Component

A comprehensive, modular WiFi Manager component for ESP-IDF projects, compatible with the popular tzapu WiFiManager API. Features a modern web interface, automatic network scanning, and robust connection management.

## 🚀 Features

- **tzapu-compatible API** - Drop-in replacement for Arduino WiFiManager
- **Modular Architecture** - Clean separation of concerns across 7 source files
- **Modern Web Interface** - Responsive design that works on all devices
- **Smart Network Scanning** - Automatic deduplication and signal strength optimization
- **Configuration Management** - JSON-based parameter system with web UI
- **Robust Connection Handling** - Automatic reconnection and timeout management
- **NVS Storage** - Persistent WiFi credentials and configuration
- **Event-driven Architecture** - Clean event handling with status callbacks

## 📁 Project Structure

```
components/wifi_manager/
├── src/                           # Source code (modular architecture)
│   ├── wifi_manager_api.c         # Public API implementation (tzapu-style)
│   ├── wifi_manager_core.c        # Core event handling and state management
│   ├── wifi_manager_scan.c        # WiFi scanning functionality
│   ├── wifi_manager_storage.c     # NVS storage operations
│   ├── wifi_manager_web.c         # HTTP server and web endpoints
│   ├── wifi_manager_config.c      # Configuration parameter management
│   └── wifi_manager_private.h     # Internal definitions and structures
├── web/                           # Web interface assets (embedded)
│   ├── setup.html                 # Main configuration page
│   ├── config.html                # Parameter configuration page
│   ├── success.html               # Connection success page
│   ├── style.css                  # Responsive CSS styling
│   └── script.js                  # Interactive JavaScript
├── wifi_manager.h                 # Public API header
├── CMakeLists.txt                 # ESP-IDF component build config
├── LICENSE                        # MIT License
└── README.md                      # This documentation
```

## 🔧 Installation

### Method 1: ESP-IDF Component Manager (Recommended)

```bash
idf.py add-dependency "pstangsdal/esp32_wifi_manager"
```

### Method 2: Git Submodule

```bash
cd your_project/components
git submodule add https://github.com/pstangsdal/esp32_wifi_manager.git wifi_manager
```

### Method 3: Manual Download

1. Download or clone this repository
2. Copy the `wifi_manager` folder to your project's `components/` directory

## 📖 Quick Start

### Basic Usage

```c
#include "wifi_manager.h"

void app_main(void)
{
    // Initialize NVS (required for WiFi storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create WiFi Manager instance
    wifi_manager_t *wm = wifi_manager_create();

    // Auto-connect to saved WiFi or start config portal
    if (!wifi_manager_auto_connect(wm, "MyESP32-Setup", NULL)) {
        ESP_LOGI("MAIN", "Failed to connect, starting config portal");
        // Config portal will run automatically
    } else {
        ESP_LOGI("MAIN", "Connected to WiFi successfully!");
    }

    // Your application code here...
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### Advanced Configuration

```c
// Set custom timeouts and parameters
wifi_manager_set_config_portal_timeout(wm, 300);  // 5 minutes
wifi_manager_set_minimum_signal_quality(wm, 20);   // 20% minimum signal

// Add custom configuration parameters
wifi_manager_add_parameter(wm, "mqtt_server", "MQTT Server", "", 64, NULL, NULL);
wifi_manager_add_parameter(wm, "mqtt_port", "MQTT Port", "1883", 6, NULL, NULL);

// Set up callbacks
wifi_manager_set_save_config_callback(wm, save_config_callback);
wifi_manager_set_ap_callback(wm, config_mode_callback);
```

## 🌐 Web Interface

The component includes a modern, responsive web interface:

- **📱 Mobile-Friendly**: Responsive design that works on all devices
- **🔍 Auto-Scan**: Automatic WiFi network discovery and refresh
- **📊 Signal Strength**: Visual indicators for network quality
- **⚙️ Configuration**: Web-based parameter management
- **🎨 Modern UI**: Clean, intuitive user experience

### Web Endpoints

| Endpoint   | Method | Description                       |
| ---------- | ------ | --------------------------------- |
| `/`        | GET    | Main setup page                   |
| `/config`  | GET    | Configuration parameters page     |
| `/wifi`    | GET    | JSON API for available networks   |
| `/connect` | POST   | WiFi connection handler           |
| `/info`    | GET    | Device and connection information |

## 🔧 Configuration Parameters

The component supports dynamic configuration parameters that can be managed via the web interface:

```c
// Add custom parameters
wifi_manager_add_parameter(wm, "device_name", "Device Name", "ESP32-Device", 32, NULL, NULL);
wifi_manager_add_parameter(wm, "update_interval", "Update Interval (s)", "30", 10, NULL, NULL);

// Parameters are automatically saved to NVS and available via web UI
```

## 📡 API Reference

### Core Functions

#### `wifi_manager_create()`

Creates and initializes a WiFi Manager instance.

```c
wifi_manager_t *wm = wifi_manager_create();
```

#### `wifi_manager_auto_connect()`

Attempts to connect to saved WiFi or starts config portal.

```c
bool wifi_manager_auto_connect(wifi_manager_t *wm, const char *ap_name, const char *ap_password);
```

#### `wifi_manager_start_config_portal()`

Manually starts the configuration portal.

```c
bool wifi_manager_start_config_portal(wifi_manager_t *wm, const char *ap_name, const char *ap_password);
```

### Configuration Functions

#### `wifi_manager_add_parameter()`

Adds a custom configuration parameter.

```c
void wifi_manager_add_parameter(wifi_manager_t *wm, const char *id, const char *label,
                                const char *default_value, int max_length,
                                const char *custom, const char *label_placement);
```

#### `wifi_manager_set_config_portal_timeout()`

Sets the config portal timeout in seconds.

```c
void wifi_manager_set_config_portal_timeout(wifi_manager_t *wm, uint32_t timeout_seconds);
```

### Callback Functions

#### `wifi_manager_set_save_config_callback()`

Sets callback for when configuration is saved.

```c
void wifi_manager_set_save_config_callback(wifi_manager_t *wm, void (*func)(void));
```

#### `wifi_manager_set_ap_callback()`

Sets callback for when AP mode starts.

```c
void wifi_manager_set_ap_callback(wifi_manager_t *wm, void (*func)(wifi_manager_t *wm));
```

## 🏗️ Architecture

The component follows a modular architecture with clear separation of concerns:

- **API Layer** (`wifi_manager_api.c`) - Public tzapu-compatible interface
- **Core Layer** (`wifi_manager_core.c`) - Event handling and state management
- **Scan Layer** (`wifi_manager_scan.c`) - Network discovery and management
- **Storage Layer** (`wifi_manager_storage.c`) - NVS persistence operations
- **Web Layer** (`wifi_manager_web.c`) - HTTP server and web interface
- **Config Layer** (`wifi_manager_config.c`) - Parameter management system

## 🐛 Troubleshooting

### Common Issues

**WiFi not connecting after setup:**

- Check signal strength in the web interface
- Verify password is correct
- Ensure the network is 2.4GHz (ESP32 doesn't support 5GHz)

**Config portal not accessible:**

- Connect to the ESP32's AP (default: ESP32-WiFi-Manager)
- Navigate to `http://192.168.4.1`
- Check firewall settings on your device

**Parameters not saving:**

- Ensure NVS is properly initialized
- Check available flash space
- Verify parameter IDs are unique

### Debug Logging

Enable verbose logging in your project:

```c
esp_log_level_set("wifi_manager", ESP_LOG_DEBUG);
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by [tzapu's WiFiManager](https://github.com/tzapu/WiFiManager) for Arduino
- Built for the ESP-IDF framework by Espressif Systems
- Community feedback and contributions

## 📈 Version History

- **v2.0.0** - Modular architecture, improved web interface, configuration management
- **v1.0.0** - Initial release with basic WiFi management functionality

4. **Standards**: Modern JavaScript (ES6+) and responsive CSS
5. **Debugging**: Proper error handling and console logging

## WiFi Features

- Automatic network scanning with event-driven architecture
- Network deduplication (strongest signal per SSID)
- Fast initial load (2-10 seconds vs 1-2 minutes)
- Persistent credential storage in NVS
- tzapu/WiFiManager compatible API
