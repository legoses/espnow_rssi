## ESP RSSI
### Requirements
- Heltec/HiLetgo ESP32 WiFi Kit V3
- Platformio must be installed. This can be adapted to work on arduino IDE, but will not as is.

### Setup
- Before uploading this, make sure to edit the config file located at `data/board.cfg`
- Use PlatformIO to upload filesystem image
- Build and upload the code to your board
- Edit the [platformio] section in the platformio.ini file to include your board. Comment out or remove any others.
- ### Encryption
    - If you wish to enable encryption, make sure ecrypt is set to 'true' in the config file
    - The 'lmk' and 'pmk' options must contain a unique 16 byte (16 character) string
    - Both of these must be the same for any board you wish to communicate with

### Knows issues
- Ocassionaly a thread will time out and cause the program to crash
