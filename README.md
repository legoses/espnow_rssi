## ESP RSSI
### Requirements
- ESP32-S3 (or possibly other ESP32 variants as well, but has only been tested on an S3)
- SSD1306 oled display
- (Optional) Heltec OLED WiFi kit V3
- A breadboard, jumper cables, or your preferred method of attaching the ESP to the display
- Platformio must be installed. This can be adapted to work on arduino IDE, but will not as is.
- Note: If you are installing heltec libraries your self: Currently the Heltec boards library you can install through platformio does not support helte V3 boards, however, heltec's github has been updated to add support. In order for V3 boards to work in platformIO, you must make these changes in order for them to work with the V3 version<br>
of the board:
    - Change directory to where ever you install your heltec library to. In platformio it will be in the .pio/libdeps/ folder in your project directory by default.
    - There may be multipe folders here, one for each board defined in your platform.io folder. The following steps need to be completed for each folder in this directory.
    - Remove the out dated heltec.cpp and heltec.h files
    - `cd <board_name>/Heltec\ ESP32\ Dev-Boards/src/; rm heltec*`
    - Download the up to date files from heltec github.
    - `wget https://raw.githubusercontent.com/HelTecAutomation/Heltec_ESP32/master/src/heltec.cpp; wget https://raw.githubusercontent.com/HelTecAutomation/Heltec_ESP32/master/src/heltec.h`
    - Open heltec.cpp in a text editor and remove the line containing 'pinMode(LED,OUTPUT);' on line 103.

### Setup
- Edit the [platformio] section in the platformio.ini file to include your board. Comment out or remove any others.
- If heltec board is being used, make sure the line '`#define heltec_wifi_kit_32_V3`', near the top of main.cpp, is not commented out or missing. If it is, add it back in.
- For other boards, remove this line.
- Edit `char userName[] = "<User name Here>";` to contain the username that will show up on other boards
- Add the MAC addresses this board will be communicating with to `char macAddr[][13]`, following the format of the example mac's the array contains

### Encryption
- ESP-NOw support encryption with up to 17 peers, but will only add up to 7 be default. In order to increase the amount, add `#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM`, followed by the number of encrypted peers you wish to support. Make sure this is near the top of the file.
- e.g. `#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM 15`.
- Set 'encryptESPNOW' to true.
- Change the values of lmk and pmk to unique 16 byte (26 character) strings. These strings must be the same on all devices you wish to communicate with.

### Knows issues
- Sometimes sorting does not work quite right
- Names will briefly appear twice when a peer is removed
