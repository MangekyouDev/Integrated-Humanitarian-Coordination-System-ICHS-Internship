# Integrated Humanitarian Coordination System (IHCS) — Internship

> ESP32-based firmware modules for a field-deployable humanitarian coordination terminal.  
> Developed as part of the IHCS internship programme.

---

## Table of Contents

- [Overview](#overview)
- [Target Hardware](#target-hardware)
- [Project Structure](#project-structure)
- [Module Details](#module-details)
  - [led_blink](#1-led_blink--gpio-blink-test)
  - [uart-output](#2-uart_output--serial-heartbeat)
  - [buttons-test](#3-buttons-test--gpio-input-with-debounce)
  - [lcd-test](#4-lcd-test--i2c-lcd-driver)
  - [spiffs_test](#5-spiffs_test--flash-file-system)
  - [combined_hardware_test](#6-combined_hardware_test--integration-of-all-subsystems)
- [Build & Flash](#build--flash)
- [Partition Tables](#partition-tables)
- [Google Drive Resources](#google-drive-resources)
- [License](#license)

---

## Overview

This repository contains a series of incremental ESP32 firmware projects built with the **ESP-IDF** (Espressif IoT Development Framework). Each sub-project is a standalone ESP-IDF component that tests or demonstrates a single hardware subsystem. The final project, `Combined_Hardware_Test`, integrates all subsystems into a single application that drives an I2C LCD, reads two push-buttons, and logs events to the SPIFFS flash file system.

The work was carried out across three internship phases:

1. **Environment Setup & Toolchain Configuration** — Getting the ESP-IDF toolchain working and flashing the first "hello world" LED blink.
2. **Hardware Bring-Up** — Individually testing UART serial output, GPIO button input, I2C LCD display, and SPIFFS persistent storage.
3. **State Machine Implementation** — Combining all peripherals into a single interactive firmware (`Combined_Hardware_Test`).

---

## Target Hardware

| Component | Details |
|---|---|
| **MCU** | ESP32-D0WD (dual-core Xtensa LX6) |
| **Framework** | ESP-IDF v6.0.1 (CMake-based build) |
| **LCD** | 16×2 character LCD via I2C (PCF8574 backpack, address `0x27`) |
| **Buttons** | 4× push-buttons (Up, Down, OK, Back), active-low with internal pull-up |
| **LED** | external LED on GPIO 33 |
| **I2C Pins** | SDA = GPIO 21, SCL = GPIO 22 |
| **Button Pins** | GPIO 25, 26, 27 (standalone test) / GPIO 32, 33 (combined test), active-low with internal pull-up |

---

## Project Structure

```
Integrated-Humanitarian-Coordination-System-IHCS-Internship/
├── Drives                          # Google Drive links for each internship phase
├── README.md                       # This file
├── led_blink/                      # Phase 1 — GPIO LED blink
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   └── main/
│       ├── CMakeLists.txt
│       └── led_blink.c
├── UART-output/                    # Phase 2 — UART serial heartbeat
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   └── main/
│       ├── CMakeLists.txt
│       └── UART-output.c
├── Buttons-test/                   # Phase 2 — GPIO button input with debounce
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   └── main/
│       ├── CMakeLists.txt
│       └── Buttons-test.c
├── LCD-test/                       # Phase 2 — I2C 16×2 LCD driver
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   └── main/
│       ├── CMakeLists.txt
│       └── LCD-test.c
├── spiffs_test/                    # Phase 2 — SPIFFS flash file system
│   ├── CMakeLists.txt
│   ├── partitions.csv
│   ├── sdkconfig
│   └── main/
│       ├── CMakeLists.txt
│       └── spiffs_test.c
└── Combined_Hardware_Test/         # Phase 3 — Integrated firmware
    ├── CMakeLists.txt
    ├── partitions.csv
    ├── sdkconfig
    └── main/
        ├── CMakeLists.txt
        └── Combined_Hardware_Test.c
```

---

## Module Details

### 1. `led_blink` — GPIO Blink Test

**Phase:** Environment Setup & Toolchain Configuration

The simplest possible ESP-IDF application. Configures GPIO 33 as an output and toggles it every 500 ms, blinking an LED. This served as the "hello world" to verify the toolchain, flashing, and serial monitor were all working correctly.

**Key details:**
- LED pin: `GPIO_NUM_33`
- Blink interval: 500 ms on / 500 ms off
- Uses FreeRTOS `vTaskDelay()` for timing

**Source:** `led_blink/main/led_blink.c`

---

### 2. `UART-output` — Serial Heartbeat

**Phase:** Hardware Bring-Up

Tests UART serial output by printing a boot message and then a incrementing "Heartbeat" counter every second. Confirms that the USB-to-UART bridge and serial monitor are functioning.

**Key details:**
- Prints `"IHCS Field Terminal — Boot OK"` at startup
- Prints `"Heartbeat: <n>"` every 1 second
- Uses `printf()` which maps to the default UART0 console

**Source:** `UART-output/main/UART-output.c`

---

### 3. `Buttons-test` — GPIO Input with Debounce

**Phase:** Hardware Bring-Up

Reads three push-buttons on GPIO 25, 26, and 27. Each button is configured as an input with an internal pull-up resistor (buttons are active-low). A simple software debounce of 200 ms is applied. On each button press, a message is printed to the serial console.

**Key details:**
- Button pins: `GPIO_NUM_25`, `GPIO_NUM_26`, `GPIO_NUM_27`
- Pull-up enabled, active-low (pressed = 0)
- Debounce delay: 200 ms
- Polling loop runs every 20 ms
- Detects falling-edge transitions (release → press)

**Source:** `Buttons-test/main/Buttons-test.c`

---

### 4. `LCD-test` — I2C LCD Driver

**Phase:** Hardware Bring-Up

Implements a bit-banged 4-bit I2C driver for a 16×2 character LCD with a PCF8574 I2C backpack (address `0x27`). The driver uses the ESP-IDF `i2c_master` API. The test displays `"IHCS OK"` on the first line and an incrementing counter on the second line, updating every second.

**Key details:**
- I2C address: `0x27`
- SDA: `GPIO_NUM_21`, SCL: `GPIO_NUM_22`
- I2C clock: 100 kHz
- 4-bit initialization sequence (`0x33 → 0x32 → 0x28 → 0x0C → 0x06 → 0x01`)
- Functions: `lcd_cmd()`, `lcd_data()`, `lcd_set_cursor()`, `lcd_print()`, `lcd_init()`
- Backlight is always on (bit 3 = `0x08` set in data byte)

**Source:** `LCD-test/main/LCD-test.c`

---

### 5. `spiffs_test` — Flash File System

**Phase:** Hardware Bring-Up

Tests the SPIFFS (SPI Flash File System) by mounting a storage partition, writing a test file, reading it back, appending a second line, and reading the full contents. This validates persistent non-volatile storage on the ESP32's flash.

**Key details:**
- Mount point: `/spiffs`
- Partition label: `storage`
- Test file: `/spiffs/report.txt`
- Operations: write (`"w"`), read (`"r"`), append (`"a"`)
- Formats flash on mount failure (`format_if_mount_failed = true`)
- Custom partition table (`partitions.csv`) with a 384 KB SPIFFS partition

**Partition layout:**

| Name | Type | SubType | Size |
|---|---|---|---|
| nvs | data | nvs | 24 KB |
| phy_init | data | phy | 4 KB |
| factory | app | factory | 1 MB |
| storage | data | spiffs | 384 KB |

**Source:** `spiffs_test/main/spiffs_test.c`

---

### 6. `Combined_Hardware_Test` — Integration of All Subsystems

**Phase:** State Machine Implementation

The culmination of the internship work. This single firmware integrates all previously tested peripherals into one application:

- **I2C LCD** — Displays status and log count on a 16×2 display
- **Two push-buttons** — Button 1 shows status on the LCD; Button 2 writes a log entry to SPIFFS
- **SPIFFS** — Persists log entries to `/spiffs/log.txt`
- **UART** — Prints debug/status messages to the serial console

**Behaviour:**

1. On boot, the system initializes SPIFFS, I2C, the LCD, and the buttons.
2. The LCD shows `"Ready"` / `"Logs: 0"`.
3. **Button 1 press:** Updates the LCD to show `"Button 1"` and the current log count.
4. **Button 2 press:** Increments the log counter, appends a timestamped entry (`"Log entry <n>"`) to `/spiffs/log.txt`, and updates the LCD to show `"Logged!"` with the new count.
5. The main loop polls buttons every 50 ms with edge detection.

**Key details:**
- Button pins: `GPIO_NUM_32` (BTN1), `GPIO_NUM_33` (BTN2)
- LCD I2C address: `0x27`, SDA = GPIO 21, SCL = GPIO 22
- Log file: `/spiffs/log.txt` (append mode)
- Custom partition table (same as `spiffs_test`)
- Reuses the full LCD driver from `LCD-test` (inlined in the same source file)

**Source:** `Combined_Hardware_Test/main/Combined_Hardware_Test.c`

---

## Build & Flash

Each sub-project is an independent ESP-IDF project. To build and flash any of them:

```bash
# 1. Set up the ESP-IDF environment (if not already done)
. $IDF_PATH/export.sh

# 2. Navigate to the desired project
cd <project_name>

# 3. Set the target chip (ESP32)
idf.py set-target esp32

# 4. Build
idf.py build

# 5. Flash to the device (adjust PORT as needed)
idf.py -p /dev/ttyUSB0 flash

# 6. Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Or combine build + flash + monitor:
idf.py -p /dev/ttyUSB0 flash monitor
```

> **Note:** Projects with a `partitions.csv` file (`spiffs_test`, `Combined_Hardware_Test`) use a custom partition table. The partition table is automatically picked up by ESP-IDF when `sdkconfig` has `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` set.

---

## Partition Tables

The `spiffs_test` and `Combined_Hardware_Test` projects use a custom partition table (`partitions.csv`):

| Name | Type | SubType | Offset | Size | Purpose |
|---|---|---|---|---|---|
| `nvs` | data | nvs | — | 0x6000 (24 KB) | Non-volatile storage (Wi-Fi config, etc.) |
| `phy_init` | data | phy | — | 0x1000 (4 KB) | PHY initialization data |
| `factory` | app | factory | — | 0x100000 (1 MB) | Application firmware |
| `storage` | data | spiffs | — | 0x60000 (384 KB) | SPIFFS file system |

---

## Google Drive Resources

The `Drives` file contains links to Google Drive folders for each internship phase:

- **Environment Setup & Toolchain Configuration:** [Drive Folder](https://drive.google.com/drive/folders/1AljFMJDrY-QZh5AD0zJDmyHB4twc0G_r?usp=drive_link)
- **Hardware Bring-Up:** [Drive Folder](https://drive.google.com/drive/folders/1NcUKMDyomdr0Bg8sfuixllj89ywi88Lt?usp=drive_link)
- **State Machine Implementation:** [Drive Folder](https://drive.google.com/drive/folders/1ZQKu4DGOcbPCFGfIWDvg8BQQo1RsRbDr?usp=drive_link)

---

## License

This project is developed as part of the IHCS internship programme. No explicit license is declared.
