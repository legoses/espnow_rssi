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

### Note on Heltec libraries
- Currently if you clone this repository, patched versions of Heltec's libraries are included. As of writing, the libraries that can be directly installed through Arduino IDE or PlatformIO are outdated and need to be patched in order to support their newer boards. Install the libraries through your ide of choice and follow these steps to patch the libraries:
    - Change directory to where ever you installed your heltec library to. In platformio it will be in the .pio/libdeps/ folder in your project directory by default.
    - There may be multiple folders here, one for each board defined in your platform.io folder. The following steps need to be completed for each folder in this directory.
    - Remove the out dated heltec.cpp and heltec.h files
    - `cd <board_name>/Heltec\ ESP32\ Dev-Boards/src/; rm heltec*`
    - Download the up to date files from heltec github.
    - `wget https://raw.githubusercontent.com/HelTecAutomation/Heltec_ESP32/master/src/heltec.cpp; wget https://raw.githubusercontent.com/HelTecAutomation/Heltec_ESP32/master/src/heltec.h`
    - Open heltec.cpp in a text editor and remove the line containing 'pinMode(LED,OUTPUT);' on line 103.


### Knows issues
- Ocassionaly a threat will time out and cause the program to crash
