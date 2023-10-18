## ESP RSSI
### Requirements
- Heltec/HiLetgo ESP32 WiFi Kit V3
- Platformio must be installed. This can be adapted to work on arduino IDE, but will not as is.

### Setup
- Edit the [platformio] section in the platformio.ini file to include your board. Comment out or remove any others.
- In the 'main.cpp' file:
    - All lines you need to edit are near the top of the file, directly under the line containing `/* USER CONFIG */`
    - Edit `char userName[] = "<User name Here>";` to contain your username. This will be what shows up on other boards you are in range of
    - ### Encryption
        - If you wish to enable encryption, set 'encryptESPNOW' equal to true
        - The 'lmk' and 'pmk' variables must contain a unique 16 byte (16 character) string
        - Both of these must be the same for any board you wish to communicate with

### Knows issues
- Ocassionaly a threat will time out and cause the program to crash
