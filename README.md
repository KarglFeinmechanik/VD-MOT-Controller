# VD-MOT Control

**VD-MOT Control** is a personal **hobby ESP32 project** for controlling a motorized valve with sensor feedback, MQTT integration, and a clean built-in web interface.

The project was developed mainly for **learning, experimenting, and practical home automation use cases** (e.g. floor heating, water flow control, or similar actuator-based systems).

---

## 📸 Web Interface Preview

<p align="center">
  <img src="https://github.com/KarglFeinmechanik/Files/blob/main/FBH_Control_ESP32_C3/data/docs/VDMOT.png" alt="VD-MOT Control Web UI" width="70%" />
</p>

> Screenshot of the built-in web interface running directly on the ESP32.

---

## ✨ Features

* ESP32-based firmware (Arduino framework)
* Motorized valve control with position tracking
* Manual control via web interface
* MQTT integration for remote control & monitoring
* Sensor support

  * DHT22 (temperature & humidity)
  * Dallas / OneWire temperature sensors
* Persistent configuration (ESP32 Preferences / NVS)
* OTA firmware updates via web UI
* LittleFS for embedded web assets
* Runtime status display (WiFi, MQTT, sensors, valve state)

---

## 🧩 Project Structure

```text
.
├── README.md
├── LICENSE
├── background.jpg
├── docs/
│   └── ui.png              # Web UI screenshot
├── main.cpp                # Application entry point
├── config.h                # Global configuration defaults
├── pins.h                  # GPIO pin definitions
├── valve.h / valve.cpp     # Valve motor control logic
├── mqtt.h / mqtt.cpp       # MQTT handling
├── webpage.h               # Embedded HTML/CSS/JS
├── version.h / version.cpp # Firmware version info
```

---

## 🔧 Valve Control

The valve logic is implemented as a dedicated module and provides:

* Open / close movement handling
* Position tracking (percentage-based)
* Stall detection
* Configurable travel time

The abstraction keeps hardware-specific details isolated and easy to modify.

---

## 🌐 Web Interface

The ESP32 hosts its own web UI which allows:

* Viewing system status (WiFi, MQTT, firmware)
* Manual valve positioning
* Sensor value display
* MQTT configuration
* Device reboot
* OTA firmware updates

No external server or cloud service is required.

---

## 📡 MQTT

MQTT is used for integration into existing home automation setups.

Typical use cases:

* Set valve target position
* Receive valve status updates
* Read temperature and humidity values

The firmware is compatible with setups like **Home Assistant** or **Node-RED**.

---

## 🌡️ Sensors

Supported sensors:

* **DHT22** – temperature & humidity
* **Dallas / OneWire** – temperature only

Sensor data is available in both the web UI and via MQTT.

---

## ⚙️ Configuration

### Compile-time

* Pin assignments (`pins.h`)
* Default settings (`config.h`)

### Runtime

* Stored in ESP32 NVS (Preferences)
* Editable via web interface
* Preserved across reboots and OTA updates

---

## 🚀 Getting Started

### Requirements

* ESP32 development board
* Arduino IDE or PlatformIO
* Required libraries:

  * WiFi
  * PubSubClient
  * ESPAsyncWebServer
  * DHTesp
  * OneWire
  * DallasTemperature

### Flashing

1. Clone this repository
2. Adjust pin configuration if needed
3. Build and flash the firmware
4. Connect to the ESP32 web interface

---

## 🔄 OTA Updates

The firmware can be updated directly from the browser using the built-in OTA feature.

This makes iteration and testing especially convenient for a hobby project.

---

## 📄 License

This project is licensed under the **MIT License**.

You are free to use, modify, and adapt the code for personal or educational purposes.

---

## ⚠️ Disclaimer

This is a **hobby project**.

No guarantees are given regarding reliability, safety, or suitability for critical systems. Use at your own risk.

---

## 🤝 Contributions

Ideas, improvements, and pull requests are welcome — especially if you are also experimenting with ESP32, MQTT, or valve control systems.


---

## 🤝 Contributing

Contributions, bug reports, and feature requests are welcome. Please open an issue or submit a pull request.

<!-- Project Banner -->
<p align="center">
  <img src="https://github.com/KarglFeinmechanik/Files/blob/main/FBH_Control_ESP32_C3/data/background.jpg" alt="VD-MOT Control Background" width="100%" />
</p>
