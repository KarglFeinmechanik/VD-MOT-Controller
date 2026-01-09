VD-MOT Control
VD-MOT Control is an ESP32-based valve and sensor control system designed for automation and remote monitoring. It combines MQTT, a web interface, and local hardware control to manage motorized valves and environmental sensors in a reliable and extensible way.
The project is intended for embedded/industrial or home‑automation scenarios such as irrigation systems, fluid control, or general actuator management.
________________________________________
✨ Features
•	ESP32 firmware written in modern Arduino-style C++
•	Motorized valve control with state tracking and safety handling
•	MQTT integration for remote control and monitoring
•	Web interface (ESPAsyncWebServer) for configuration and status display
•	Sensor support
o	DHT temperature & humidity sensors
o	Dallas/OneWire temperature sensors
•	Persistent configuration using ESP32 Preferences
•	Over-the-Air (OTA) firmware updates via web interface
•	LittleFS for serving web assets
•	Versioning & build identification embedded in firmware
________________________________________
🧩 Project Structure
.
├── main.cpp          # Application entry point and system orchestration
├── config.h          # Global configuration (WiFi, MQTT, system options)
├── pins.h            # GPIO pin assignments        
├── valve.h / valve.cpp
│                     # Valve abstraction and motor control logic
├── mqtt.h / mqtt.cpp # MQTT client setup and message handling
├── webpage.h         # Embedded HTML/JS for the web UI
├── version.h / version.cpp
│                     # Firmware version and build metadata
└── data/ (optional)  # LittleFS content (if extended)
________________________________________
⚙️ Core Components
Valve Control
The Valve module encapsulates:
•	Opening and closing logic for motorized valves
•	State handling (open / closed / moving)
•	Timing and safety constraints
This abstraction allows the firmware to remain hardware-agnostic while keeping the control logic centralized.
________________________________________
MQTT Communication
The MQTT module provides:
•	Connection handling to an MQTT broker
•	Topic-based command reception
•	Status publishing (valve state, sensor values, system info)
This enables seamless integration into systems like Home Assistant, Node-RED, or custom backends.
________________________________________
Web Interface
The embedded web server offers:
•	Live system status
•	Manual valve control
•	Network and system information
•	OTA firmware upload
The web UI is served directly from flash using LittleFS, keeping the system fully self-contained.
________________________________________
Sensors
Supported sensors include:
•	DHT series for temperature and humidity
•	Dallas OneWire sensors for accurate temperature measurement
Sensor data can be exposed both via MQTT and the web interface.
________________________________________
🔧 Configuration
Configuration is split into two layers:
1.	Compile-time configuration (config.h, pins.h)
o	GPIO assignments
o	Default system parameters
2.	Runtime configuration
o	Stored in ESP32 Preferences (NVS)
o	Preserved across reboots and firmware updates
________________________________________
🚀 Getting Started
Requirements
•	ESP32 development board
•	Arduino IDE or PlatformIO
•	Required libraries:
o	WiFi
o	PubSubClient
o	ESPAsyncWebServer
o	DHTesp
o	OneWire
o	DallasTemperature
Build & Flash
1.	Clone the repository
2.	Configure pins and defaults in config.h and pins.h
3.	Build and upload the firmware to the ESP32
4.	Connect to the web interface or MQTT broker
________________________________________
🔄 OTA Updates
The firmware supports web-based OTA updates, allowing you to:
•	Upload new firmware directly via the browser
•	Avoid physical access after deployment
________________________________________
🧪 Versioning
Each build includes:
•	Firmware version
•	Compile date and time
This information is accessible via MQTT and the web interface for easy diagnostics.
________________________________________
📌 Use Cases
•	Irrigation and watering systems
•	Industrial or hobby fluid control
•	Smart home actuator integration
•	Remote-controlled valve systems
________________________________________
🛠️ Future Extensions
Possible enhancements include:
•	Additional valve channels
•	Role-based web authentication
•	Encrypted MQTT (TLS)
•	Scheduler / automation rules
________________________________________
📄 License
This project is released under an open-source license. See the LICENSE file for details.
________________________________________
🤝 Contributing
Contributions, bug reports, and feature requests are welcome. Please open an issue or submit a pull request.
