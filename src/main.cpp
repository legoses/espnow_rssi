/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

/*
  TODO:
  Get the macs and flash last 5 boards
  Fix sorting
  figure out why new boards appear twice on other devices
*/

#include <Arduino.h>
#include <badge_images.h>
#include <display_username.h>
#include <listener.h>
#include <displayinfo.h>

#define heltec_wifi_kit_32_V3
#define USE_MUTEX
#define ARDUINO_RUNNING_CORE 1

#include <Wire.h>
#include "esp_now.h"
#include "esp_wifi.h"

#ifdef heltec_wifi_kit_32_V3
# include "heltec.h"
#define Fbattery 3700;
float XS = 0.0025;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;
#else
# include <Adafruit_SSD1306.h>
# include <Adafruit_GFX.h>
# define SCREEN_WIDTH 128
# define SCREEN_HEIGHT 64
# define OLED_RESET -1
# define I2C_SDA 39
# define I2C_SCL 38
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

PeerListener listener;
DisplayInfo displayInfo;

//Change to true to enable encryption
bool encryptESPNOW = false;

//Each of these must contain a 16 byte string
static const char *pmk = "<PMK here>";
static const char *lmk = "<LMK here>";

//Username you want to show up on other displays
char userName[] = "board 2";

//Insert the MAC addresses of the boards this board will be communicating with
//Insert mac address ad string, removing colons
//origional macs
char macAddr[][13] = {
  {"F412FA745B2C"}, // board 1
  //{"F412FA744A5C"}, // board 2
  {"F412FA745B84"}, // board 3
  {"F412FA75F1D0"}, // board 4
  {"F412FA744AA4"}, // board 5
  {"F412FA744C38"}, //board 6
  {"F412FA74DE00"}, //board 7
  {"F412FA74492C"}, //board 8
  {"F412FA7449F8"}, //board 9
  {"F412FA745B0C"}, //board 10
};
/*
//spoofed macs
char macAddr[][13] = {
  // {"8E5CDAEE1697"}, Example mac
  // {"0504A4C587AF"}  Example mac
  //{"f412fa815118"}, //Eric's mac
  //{"F412FA66EB00"}, //Kyle's mac
  //{"f412fa66e9ec"} // Paul's mac
  {"fc2f464d963f"}, //board 1
  //{"EA8CD317B388"}, //board 2
  {"8E2267184D34"}, //board 3
  {"0A59C6EF3809"}, //board 4
  {"94AE8D793ABF"}, //board 5
  {"829E188E3507"}, //board 6
  {"74FC2F78DB4F"}, //board 7
  {"F2BF50A0AEE9"}, //board 8
  "28973258A0B5"}, //board 9
  {"7A449F4784C6"}, //board 10
  //{"7CDFA1E403AC"}, //non heltec
};
*/
//const uint8_t boardMac[6] = {0xfc, 0x2f, 0x46, 0x4d, 0x96, 0x3f}; //board 1
//const uint8_t boardMac[6] = {0xCE, 0xCF, 0x63, 0x23, 0xE4, 0x1E}; //board 1
//const uint8_t boardMac[6] = {0xEA, 0x8C, 0xD3, 0x17, 0xB3, 0x88}; //board 2
//const uint8_t boardMac[6] = {0x8E, 0x22, 0x67, 0x18, 0x4D, 0x34}; //board 3
//const uint8_t boardMac[6] = {0x0A, 0x59, 0xC6, 0xEF, 0x38, 0x09}; //board 4
//const uint8_t boardMac[6] = {0x94, 0xAE, 0x8D, 0x79, 0x3A, 0xBF}; //board 5
//const uint8_t boardMac[6] = {0x82, 0x9E, 0x18, 0x8E, 0x35, 0x07}; //board 6
//const uint8_t boardMac[6] = {0x74, 0xFC, 0x2F, 0x78, 0xDB, 0x4F}; //board 7
//const uint8_t boardMac[6] = {0xF2, 0xBF, 0x50, 0xA0, 0xAE, 0xE9}; //board 8
//const uint8_t boardMac[6] = {0x28, 0x97, 0x32, 0x58, 0xA0, 0xB5}; //board 9
//const uint8_t boardMac[6] = {0xDC, 0x7D, 0x7F, 0xFA, 0x21, 0xB8}; //board 10
//const uint8_t boardMac[6] = {0x7A, 0x44, 0x9F, 0x47, 0x84, 0xC6}; //board 10

const int SCREEN_REFRESH = 2500;

//Hold mac address once parsed
uint8_t broadcastAddress[20][6];

int numCurPeer = 0;

int macNum = sizeof(macAddr) / sizeof(macAddr[0]);
long timeSinceLastLogo = 0;

/*
//Hold info to send and recieve data
typedef struct struct_message {
  int8_t rssi;
  char userName[32];
} struct_message;

//struct_message sendMessage;
struct_message recvMessage;
*/


esp_now_peer_info_t peerInfo;

int lineCount = 0;
int file = 0;

//Create mutex to lock objects and prevent race conditions
SemaphoreHandle_t xMutex = NULL;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  //Serial.print("\r\nLast packet send status: ");
  ////Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failed");
}


void init_wifi()
{
  
  //set default wifi config as recommended by docs
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  /* 
  esp_err_t setMac = esp_wifi_set_mac(WIFI_IF_STA, boardMac);
  if(setMac != ESP_OK)
  {
    ////Serial.println("Unable to init wifi");
    switch(setMac)
    {
      case ESP_ERR_WIFI_NOT_INIT:
        ////Serial.println("Wifi not init");
        break;
      case ESP_ERR_INVALID_ARG:
        ////Serial.println("Invalid args");
        break;
      case ESP_ERR_WIFI_IF:
        ////Serial.println("Invalid interface");
        break;
      case ESP_ERR_WIFI_MAC:
        ////Serial.println("Invalid mac");
        break;
      case ESP_ERR_WIFI_MODE:
        ////Serial.println("Wifi mode is wrong");
        break;
      default:
        ////Serial.println("You did it wrong dumbass");
    }
    return;
  }
  */
  esp_wifi_set_mode(WIFI_MODE_STA);

  esp_wifi_start();
}


//Load espnow peers
void addPeerInfo()
{
  //Set channel
  peerInfo.channel = 0;
  
  uint8_t hexVal;
  char tmpStr[2];

  //Iterate through mac addresses
  for(int j = 0; j < macNum; j++)
  {
    //Parse mac address and convert from string to hex value
    for(int i = 0; i < 12; i++)
    {
      if((i % 2 == 0) || (i == 0))
      {
        tmpStr[0] = macAddr[j][i];
        tmpStr[1] = macAddr[j][i+1];
        hexVal = (uint8_t)strtol(tmpStr, NULL, 16);
        broadcastAddress[j][i/2] = hexVal;
      }
    }
  memcpy(peerInfo.peer_addr, broadcastAddress[j], 6);

  for(int i = 0; i < 16; i++)
  {
    peerInfo.lmk[i] = lmk[i];
  }
  peerInfo.encrypt = encryptESPNOW;
  esp_now_add_peer(&peerInfo);
  }
}

//Display logos on screen
void displayLogos()
{
  int screenEdgeBuf = ((img_buffer / 2) * -1) + 10;
  int pimaLogoDelay = 0;

  for(int i = 28; i > screenEdgeBuf; i-=2)
  {
    Heltec.display->clear();
    Heltec.display->drawXbm(i, 0, 71, 62, pima_logo);
    Heltec.display->display();
    delay(25);
    pimaLogoDelay+=25;
  }
  delay(2000-pimaLogoDelay);
  Heltec.display->clear();
  Heltec.display->drawXbm(screenEdgeBuf, 0, 71, 62, range_logo);
  Heltec.display->display();
  delay(2000);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 10, "[UserName]");
  Heltec.display->drawString(64, 20, userName);
  Heltec.display->display();
  delay(1000);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
}

//Handle espnow packets
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  ////Serial.println("DATA RECIEVED");
  listener.dataRecv(mac, incomingData);
}

//Handle any incoming packets
void promiscuousRecv(void *buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
  int8_t packetRssi = rssiInfo->rx_ctrl.rssi;
  listener.promiscuousRecv(packetRssi);
}

/*
void dispBat()
{
  uint16_t c  =  analogRead(20);
  //uint16_t c  =  analogRead(13)*0.769 + 150;
  double voltage = (c * 5.0) / 1024.0;

  //Serial.print("Battery POWER: ");
  //////Serial.println(voltage);
  ////Serial.println(analogRead(20));
  ////Serial.println(voltage);
  ////Serial.println();
  
  char tempBatPow[10];
  snprintf(tempBatPow, sizeof(voltage), "%d", voltage);
  int batStrLen = Heltec.display->getStringWidth(tempBatPow, strlen(tempBatPow));
  Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
  Heltec.display->drawString(128, 0, (String)voltage);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
}
*/


void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);
  xMutex = xSemaphoreCreateMutex();

  //Set username and check size
  if(listener.setUserName(userName, sizeof(userName)) != 0)
  {
    ////Serial.println("[ERROR] UserName too large");
    return;
  }

  //analogSetClockDiv(1);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255
  //analogSetAttenuation(ADC_11db);       // Sets the input attenuation for ALL ADC inputs, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
  analogSetPinAttenuation(20,ADC_11db); // Sets the input attenuation, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
  //analogSetPinAttenuation(36,ADC_11db);
  adcAttachPin(20);
  //adcAttachPin(11);
  //pinMode(20, INPUT);

  //Init display
#ifdef heltec_wifi_kit_32_V3
  Heltec.begin(true, false, false);
  Heltec.display->init();
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
 
  //displayUsername(userName);
  
  displayLogos();

#else
  //Setup i2c connection 
  Wire.begin(I2C_SDA, I2C_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    ////Serial.println("Display allocation failed");
  } 

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.clearDisplay(); 
  delay(100);
  display.println("Waiting for communication...");
  display.display();
#endif

  //Serial.print("setup() running on core ");
  //memcpy(sendMessage.userName, userName, 32);
  ////Serial.println(xPortGetCoreID());

  init_wifi();

  if(esp_now_init() != ESP_OK)
  {
    ////Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_pmk((uint8_t*)pmk);
  addPeerInfo();

  //Tasks to run on second core
  //When creating these functions, make sure they have some sort of return or vTaskDelete90, even if they are void
  //Otherwise the program will crash

  xTaskCreatePinnedToCore(
    handleDisplay
    , "Print peers"
    , 8192
    , NULL
    , 1
    , NULL
    , 0
  );

  xTaskCreatePinnedToCore(
    checkForDeadPeers
    , "Check dead peers"
    , 4096
    , NULL
    , 2
    , NULL
    , 0
  );

  //Register funciton to be called every time an esp-now packet is revieved
  if(esp_now_register_recv_cb(onDataRecv) != ESP_OK)
  {
    ////Serial.println("ERRORERRORERROERROORERRORERROROERRORO");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  //Set adaptor to promiscuous mode in order to recieve connection info, and register callback function
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRecv);
}



void loop() {
  listener.send_esp();
  //Setting first value to NULL will send data to all registered peers
  //esp_err_t result = esp_now_send(NULL, (uint8_t*)&sendMessage, sizeof(sendMessage));
  delay(2000);
  ////Serial.println();
}


void checkForDeadPeers(void* pvParameters)
{
  //I don't think this does anything since I don't pass any parameter to this funciton
  //Possibly delete
  (void)pvParameters;

  while(true)
  {
    delay(10000);
    ////Serial.println("[INFO] Checking for dead peers");
    //Serial.printf("[INFO] Peers: %i\n", listener.getNumCurPeer());

    for(int i = 0; i < listener.getNumCurPeer(); i++)
    {  
      //Serial.printf("[INFO] Time last seen: %i\n", listener.getTimeLastSeen(i));
      long timeLastSeen = listener.getTimeLastSeen(i);
      if((millis() - timeLastSeen > 10000) && timeLastSeen != 0)
      {
        //Serial.printf("Removing Peer %i\n", i);
          if(xMutex != NULL)
          {
            if(xSemaphoreTake(xMutex, listener.getMaxDelay()) == pdTRUE)
            {
              listener.removeDeadPeer(i);
              listener.removePeer();
            }
            xSemaphoreGive(xMutex);
        } 
      }
      else if(timeLastSeen == 0)
      {
        break;
      }
    }
  }
  return;
}


void checkLogoTime()
{
  if(millis() - timeSinceLastLogo > 25000)
  {
    clearScreen();

    displayLogos();
    timeSinceLastLogo = millis();
  }
}
 

//Print out peers to the display
void handleDisplay(void* pvParameters)
{
  (void)pvParameters;
  
  while(true)
  {
    if(xMutex != NULL)
    {
      delay(SCREEN_REFRESH);
      checkLogoTime();

      if(xSemaphoreTake(xMutex, listener.getMaxDelay()) == pdTRUE)
      {
        int peers = listener.getNumCurPeer();
        //Load list into local array and sort
        ////Serial.println("Loading arrays...");
        displayInfo.updatePeers();
        ////Serial.println("Sorting arrays...");
        displayInfo.sortPeers();

//Display peers on heltec
#ifdef heltec_wifi_kit_32_V3
        //Heltec.display->clear();
        //displayUsername(userName);
        Heltec.display->setColor(BLACK);
        Heltec.display->fillRect(0, 0, 100, 64);
        Heltec.display->setColor(WHITE);
        
        //dispBat();
        int yCursorPos = 5;
        char tmpRssi[6];
        Serial.print("PEERS: ");
        Serial.println(peers);
        
        if(peers == 0)
        {
          Heltec.display->drawString(0, yCursorPos, "You Are Alone.");
        }
        //Scroll through peers if there are too many to fit on the screen
        else if(peers > 5)
        {
          Serial.println("Peer loop: ");
          for(int i = 0; i < peers-4; i++)
          {
            Heltec.display->clear();
            //Serial.println(i);
            for(int j = i; j < i+5; j++)
            {
              Serial.println(j);
              char *tempUserName = displayInfo.getUserName(j);
              //Serial.printf("Username: %s\n", tempUserName);
              snprintf(tmpRssi, 5, "%d", displayInfo.getRssi(j));

              char placeNum[10];
              snprintf(placeNum, 10, "%i. ", j+1);
              ////Serial.println(placeNum);
              
              Heltec.display->drawString(0, yCursorPos, placeNum);
              Heltec.display->drawString(15, yCursorPos, tempUserName);
              Heltec.display->drawString(80, yCursorPos, tmpRssi);
              yCursorPos+=10;
              
            }
            Heltec.display->display();
            delay(1500);
            yCursorPos = 5;
          }
          Serial.println("reverse");
          //Display peers is reverse order
          for(int i = peers; i > 5; i--)
          {
            
            Heltec.display->clear();
            for(int j = i-5; j < i; j++)
            {
              char *tempUserName = displayInfo.getUserName(j);
              //Serial.printf("Username: %s\n", tempUserName);
              snprintf(tmpRssi, 5, "%d", displayInfo.getRssi(j));

              char placeNum[10];
              snprintf(placeNum, 10, "%i. ", j+1);
              ////Serial.println(placeNum);
              
              Heltec.display->drawString(0, yCursorPos, placeNum);
              Heltec.display->drawString(15, yCursorPos, tempUserName);
              Heltec.display->drawString(80, yCursorPos, tmpRssi);
              yCursorPos+=10;
              
            }
            Heltec.display->display();
            delay(1500);
            yCursorPos = 5;
          }
        }
        else
        {
          for(int i = 0; i < peers; i++)
          {
            char *tempUserName = displayInfo.getUserName(i);
            //Serial.printf("Username: %s\n", tempUserName);
            snprintf(tmpRssi, 5, "%d", displayInfo.getRssi(i));

            char placeNum[10];
            snprintf(placeNum, 10, "%i. ", i+1);
            ////Serial.println(placeNum);

            Heltec.display->drawString(0, yCursorPos, placeNum);
            Heltec.display->drawString(15, yCursorPos, tempUserName);
            Heltec.display->drawString(80, yCursorPos, tmpRssi);
            yCursorPos+=10;
          }
        }
        Heltec.display->display();

//Display peers on ssd1306
#else
        display.clearDisplay();
        display.setCursor(0, 10);

        if(listener.getNumCurPeer() == 0)
        {
          display.println("You Are Alone.");
        }
        else
        {
          for(int i = 0; i < listener.getNumCurPeer(); i++)
          {
            display.print(sortUserNameList[i]);
            display.print("   ");
            display.println(rssiArr[i]);
          }
        }
        display.display();
#endif
      }
      xSemaphoreGive(xMutex);
    } 
  }
  return;
}
