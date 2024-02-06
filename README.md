# ESP32 Sound Detector and Notifier

Detect sound using ESP32 and notify over WiFi
- `esp32_sound_detector` - ESP32 code for detecting noise using KY-038 and sending a UDP packet over WiFi
- `notifier` - Python server that waits for the UDP packet and sends a desktop notification

## Building
1. Install [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html)
    - An AUR package for Arch Linux is available [esp-idf](https://aur.archlinux.org/packages/esp-idf)
2. Install toolchain
    ```bash
    # By default installs to $HOME/.espressif
    /opt/esp-idf/install.sh esp32
    ```
3.  Load environment variables
    ```bash
    . /opt/esp-idf/export.sh
    ```
4. Build and flash using `idf.py` commands
    ```bash
    cd esp32_sound_detector

    # Set the basic esp32 as target device
    # Not required on an existing preconfigured project such as this one
    idf.py set-target esp32

    # Configure the "WiFi Connection" and "Sound Notification" sections
    # Define the IP and Port to send the UDP packet to
    # Define secret to filter out stray unrelated packets
    #   (If you're sending packets to a broadcast IP the secret can be easily discovered in the LAN)
    # Menuconfig is also be used for switching features and flags such as log level
    idf.py menuconfig

    # Build
    # Run the wrapper script instead of `idf.py build` for better LSP support
    ./build.sh

    # If you encounter a permission problems under Arch, make sure you are part of the uucp group
    sudo usermod -a -G uucp $USER
    newgrp uucp # or reboot

    # Connect the device and flash it, USB serial should be detected automatically
    # flashing will also rebuild automatically if files have changed
    idf.py flash

    # Read ESP32 serial output
    idf.py monitor
    ```

## Notifier Server Setup
1. Make sure `libnotify` is installed as `notify-send` is used
2. Make sure a notification server such as `dunst` is installed
3. Test the script with parameters you have configured in the ESP
    ```bash
    # Listen on 192.168.1.1:12100 for packets with secret "ABC" and send the notification "GotIt"
    python notifier/notifier.py 192.168.1.1 12100 ABC GotIt

    # Listen on broadcast IP 192.168.1.255:10000 for packets with secret "Secret" and send the notification "OK"
    python notifier/notifier.py 192.168.1.255 10000 Secret OK
    ```
4. To setup a SystemD service, copy the supplied template file `notifier/sound-notifier.service.template` and edit the parameters
