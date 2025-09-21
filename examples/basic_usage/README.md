# Basic WiFi Manager Usage Example

This example demonstrates the simplest way to use the ESP32 WiFi Manager component.

## What This Example Does

1. **Initialize NVS Flash** - Required for storing WiFi credentials
2. **Create WiFi Manager** - Initialize the component
3. **Configure Parameters** - Set timeouts, signal quality, and custom parameters
4. **Auto-Connect** - Automatically connect to saved WiFi or start config portal
5. **Handle Results** - Process connection success or failure

## Key Features Demonstrated

- ✅ **Auto-connection** to previously saved WiFi credentials
- ✅ **Config portal** automatically starts if no saved credentials or connection fails
- ✅ **Custom parameters** that appear in the web interface
- ✅ **Callbacks** for configuration events
- ✅ **Timeout management** for the configuration portal

## How to Use

1. **Flash the example** to your ESP32
2. **Monitor the serial output** to see the connection process
3. **If no WiFi is saved:**
   - Connect to the ESP32's WiFi AP (default: "ESP32-Setup")
   - Open a web browser and go to `http://192.168.4.1`
   - Select your WiFi network and enter the password
   - Configure any custom parameters
   - Click "Save" to connect

## Expected Output

```
I (123) BASIC_EXAMPLE: Basic WiFi Manager Example Starting...
I (456) BASIC_EXAMPLE: NVS flash initialized
I (789) BASIC_EXAMPLE: WiFi Manager created successfully
I (012) BASIC_EXAMPLE: Starting WiFi Manager auto-connect...
I (345) BASIC_EXAMPLE: ✅ Successfully connected to WiFi!
I (678) BASIC_EXAMPLE: 🌐 Device is now online and ready for your application
I (901) BASIC_EXAMPLE: 🚀 Basic WiFi Manager example completed
```

## Customization

You can customize this example by:

- **Adding more parameters** with `wifi_manager_add_parameter()`
- **Changing the AP name** in `wifi_manager_auto_connect()`
- **Modifying timeouts** with `wifi_manager_set_config_portal_timeout()`
- **Adding your application logic** in the main loop

## Next Steps

Once you understand this basic example, you can:

- Explore the advanced example for more features
- Integrate the WiFi Manager into your own project
- Customize the web interface styling and functionality
