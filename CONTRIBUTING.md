# Contributing to ESP32 WiFi Manager

Thank you for your interest in contributing to the ESP32 WiFi Manager component! 🎉

## Ways to Contribute

- 🐛 **Report bugs** - Help us identify and fix issues
- 💡 **Suggest features** - Share ideas for improvements
- 📝 **Improve documentation** - Fix typos, add examples, clarify instructions
- 🔧 **Submit code** - Bug fixes, new features, optimizations
- 🧪 **Test** - Try the component in different environments and report results

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/yourusername/esp32_wifi_manager.git
   cd esp32_wifi_manager
   ```
3. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Prerequisites
- ESP-IDF v4.4 or later
- ESP32 development board
- WiFi network for testing

### Building and Testing
```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Create test project
mkdir test_project && cd test_project
idf.py create-project wifi_manager_test

# Copy component
cp -r ../path/to/wifi_manager components/

# Build and test
idf.py build
idf.py flash monitor
```

## Code Style

### C Code Standards
- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 120 characters
- **Naming**: 
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Structs: `snake_case_t`

### Documentation
- Use Doxygen-style comments for all public functions
- Include parameter descriptions and return values
- Add usage examples for complex functions

### Example:
```c
/**
 * @brief Connect to WiFi network with automatic retry
 * 
 * @param wm WiFi Manager instance
 * @param ssid Network name to connect to
 * @param password Network password (can be NULL for open networks)
 * @param timeout_ms Maximum time to wait for connection (0 = no timeout)
 * @return ESP_OK on success, error code on failure
 * 
 * @example
 * ```c
 * wifi_manager_t *wm = wifi_manager_create();
 * esp_err_t result = wifi_manager_connect(wm, "MyNetwork", "password123", 30000);
 * if (result == ESP_OK) {
 *     ESP_LOGI("MAIN", "Connected successfully!");
 * }
 * ```
 */
esp_err_t wifi_manager_connect(wifi_manager_t *wm, const char *ssid, 
                              const char *password, uint32_t timeout_ms);
```

## Testing Guidelines

### Before Submitting
1. **Build test**: Ensure code compiles without warnings
2. **Functional test**: Test basic WiFi connection functionality
3. **Memory test**: Check for memory leaks during connect/disconnect cycles
4. **Multiple targets**: Test on different ESP32 variants if possible

### Test Cases to Consider
- Fresh device (no saved credentials)
- Device with saved credentials (successful connection)
- Device with saved credentials (network not available)
- Multiple networks with same SSID
- Weak signal conditions
- Configuration portal timeout scenarios
- Custom parameter handling

## Submitting Changes

### Pull Request Process
1. **Update documentation** if you're changing functionality
2. **Add/update tests** for new features
3. **Update CHANGELOG.md** with your changes
4. **Ensure CI passes** (builds successfully on all targets)
5. **Submit pull request** with clear description

### Pull Request Template
```markdown
## Description
Brief description of changes made

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Tested on ESP32
- [ ] Tested on ESP32-S3 (if applicable)
- [ ] No memory leaks detected
- [ ] Configuration portal works correctly
- [ ] Auto-connection works correctly

## Checklist
- [ ] My code follows the style guidelines
- [ ] I have performed a self-review of my code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
```

## Component Architecture

Understanding the modular architecture will help you contribute effectively:

```
src/
├── wifi_manager_api.c      # Public tzapu-compatible API
├── wifi_manager_core.c     # Event handling and state management
├── wifi_manager_scan.c     # Network discovery and scanning
├── wifi_manager_storage.c  # NVS persistence operations
├── wifi_manager_web.c      # HTTP server and web interface
├── wifi_manager_config.c   # Configuration parameter management
└── wifi_manager_private.h  # Internal definitions
```

### Adding New Features
- **API changes**: Modify `wifi_manager_api.c` and update `wifi_manager.h`
- **Web interface**: Update files in `web/` directory
- **Configuration**: Add parameters via `wifi_manager_config.c`
- **Storage**: NVS operations in `wifi_manager_storage.c`

## Documentation

### README Updates
When adding features, update the main README.md with:
- New API functions in the reference section
- Usage examples
- Configuration options

### Code Documentation
- Document all public functions with Doxygen comments
- Include parameter validation and error handling notes
- Add usage examples for complex functionality

## Release Process

### Version Numbers
- **Major (X.0.0)**: Breaking API changes
- **Minor (X.Y.0)**: New features, backwards compatible
- **Patch (X.Y.Z)**: Bug fixes, small improvements

### Maintainer Process
1. Update version in `idf_component.yml`
2. Update `CHANGELOG.md`
3. Create git tag: `git tag v2.1.0`
4. Push tag: `git push --tags`
5. Create GitHub release

## Getting Help

- 📖 **Documentation**: Check README.md and inline comments
- 💬 **Discussions**: Use GitHub Discussions for questions
- 🐛 **Issues**: Create an issue for bugs or feature requests
- 📧 **Contact**: Reach out to maintainers for complex questions

## Code of Conduct

### Our Standards
- **Be respectful** and inclusive in all interactions
- **Provide constructive feedback** on code and ideas
- **Help newcomers** learn and contribute
- **Focus on the technical merit** of contributions

### Unacceptable Behavior
- Harassment, discrimination, or offensive language
- Personal attacks or trolling
- Publishing private information without permission
- Any behavior that would be inappropriate in a professional setting

## Recognition

Contributors will be recognized in:
- README.md contributors section
- Release notes for significant contributions
- GitHub contributor statistics

Thank you for helping make ESP32 WiFi Manager better! 🚀