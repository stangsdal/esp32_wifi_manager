# Changelog

All notable changes to the ESP32 WiFi Manager component will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2025-09-21

### 🎉 Major Release - Complete Rewrite

This is a major rewrite of the WiFi Manager component with significant improvements in architecture, functionality, and user experience.

### Added

- **Modular Architecture**: Split into 7 focused source files for better maintainability
- **Modern Web Interface**: Responsive design with HTML, CSS, and JavaScript separation
- **Configuration Management**: JSON-based parameter system with web UI
- **Smart Network Scanning**: Automatic deduplication and conflict resolution
- **Event-driven Core**: Clean event handling with status callbacks
- **tzapu API Compatibility**: Drop-in replacement for popular Arduino WiFiManager
- **Robust Connection Handling**: Improved timeout management and auto-reconnection
- **NVS Storage Integration**: Persistent configuration and credential storage
- **Multiple Target Support**: ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C6
- **Comprehensive Documentation**: Detailed README, examples, and API reference
- **Component Manager Support**: IDF Component Manager manifest for easy installation

### Improved

- **Performance**: Faster network scanning and connection establishment
- **Memory Usage**: Optimized memory allocation and embedded web assets
- **User Experience**: Intuitive web interface with real-time updates
- **Code Quality**: Clean separation of concerns and comprehensive error handling
- **Debugging**: Enhanced logging and troubleshooting information

### Technical Highlights

- **Scan Task Architecture**: Dedicated background scanning task to prevent conflicts
- **Web Asset Embedding**: All HTML, CSS, and JS embedded at build time
- **Parameter Validation**: Input validation and sanitization for all user data
- **Signal Strength Optimization**: Automatic selection of strongest AP per SSID
- **Captive Portal Detection**: Improved portal detection and redirection

### Files Structure

```
src/
├── wifi_manager_api.c      # Public tzapu-compatible API
├── wifi_manager_core.c     # Event handling and state management
├── wifi_manager_scan.c     # Network discovery and scanning
├── wifi_manager_storage.c  # NVS persistence operations
├── wifi_manager_web.c      # HTTP server and web interface
├── wifi_manager_config.c   # Configuration parameter management
└── wifi_manager_private.h  # Internal definitions

web/
├── setup.html             # Main configuration page
├── config.html            # Parameter configuration page
├── success.html           # Connection success page
├── style.css              # Responsive styling
└── script.js              # Interactive functionality
```

### Migration from v1.x

- Update include path: `#include "wifi_manager.h"`
- API remains compatible with tzapu WiFiManager
- Configuration parameters now managed via dedicated API
- Web interface files moved to `web/` directory

### Breaking Changes

- Removed legacy configuration methods
- Updated internal structure definitions
- Changed some callback function signatures

### Known Issues

- None reported in this release

## [1.0.0] - 2025-09-01

### Added

- Initial release with basic WiFi management functionality
- Simple web interface for network selection
- NVS storage for WiFi credentials
- Basic configuration portal

### Features

- WiFi network scanning and connection
- Access Point mode for configuration
- Persistent credential storage
- Basic web interface

---

## Development Guidelines

### Version Numbering

- **Major (X.0.0)**: Breaking changes, major architecture changes
- **Minor (X.Y.0)**: New features, backwards compatible
- **Patch (X.Y.Z)**: Bug fixes, small improvements

### Release Process

1. Update version in `idf_component.yml`
2. Update changelog with new features and fixes
3. Tag release in git: `git tag v2.0.0`
4. Push tags: `git push --tags`
5. Create GitHub release with changelog notes
