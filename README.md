# Sound Detector

Detect sound using ESP32 and notify over WiFi
- `esp32_sound_detector` - ESP32 Module for detecting noise using KY-038 and sending a UDP packet
- `sound_notifier` - Python code that awaits the UDP packet and sends a desktop notification

## Building
1. Install [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html)
    - An AUR package for Arch Linux is available [esp-idf]https://aur.archlinux.org/packages/esp-idf
2. Install toolchain
```bash
# By default installs to $HOME/.espressif
/opt/esp-idf/install.sh esp32
3.  Load environment variables
```bash
source /opt/esp-idf/export.sh
```
4. Use idf.py to issue build and flashing commands
```bash
cd esp32_sound_detector

# Set the basic esp32 as target device
# Not required on an existing preconfigured project such as this one
# idf.py set-target esp32

# Use for switching features on and off such as log level
idf.py menuconfig

# Build
idf.py build

# Flash to device, flashing will also rebuild automatically if files have changed
idf.py flash

# Read ESP32 serial output
idf.py monitor
```
