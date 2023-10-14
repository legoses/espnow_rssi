/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

/*
  TODO:
  Try to get espnow to broadcast to all devices in the area instead of specific whitelisted device.
  Espnow does not support multicast encrypted broadcasting. Going to try changing the mac of each board to a single address and broadcasting to that

  Currently this seems to work, but I will need to test with more boards
*/

#include <Arduino.h>
#include <badge_images.h>
#include <display_username.h>
#include <listener.h>
#include <displayinfo.h>
#include <esp_heap_caps.h>
#include "SPIFFS.h"

#define heltec_wifi_kit_32_V3
#define USE_MUTEX
#define ARDUINO_RUNNING_CORE 1

#include <Wire.h>
#include "esp_now.h"
#include "esp_wifi.h"

#ifdef heltec_wifi_kit_32_V3
# include "heltec.h"
#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM 10
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
bool encryptESPNOW = true;

//Each of these must contain a 16 byte string
char pmk[17];
char lmk[17];

//Username you want to show up on other displays
//char userName[] = "legoses";
char userName[33];

//Insert the MAC addresses of the boards this board will be communicating with
//Insert mac address ad string, removing colons
//origional macs
char macAddr[][13] = {
  {"F412FA66EB00"}, // global mac
};
const int SCREEN_REFRESH = 2500;

//Hold mac address once parsed
uint8_t broadcastAddress[20][6];

int numCurPeer = 0;

int macNum = sizeof(macAddr) / sizeof(macAddr[0]);
long timeSinceLastLogo = 0;

esp_now_peer_info_t peerInfo;

int lineCount = 0;
int file = 0;

//Create mutex to lock objects and prevent race conditions
SemaphoreHandle_t xMutex = NULL;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast packet send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failed");
}


void init_wifi()
{
  const uint8_t broadcastMac[6] = {0xF4, 0x12, 0xFA, 0x66, 0xEB, 0x00};
  //set default wifi config as recommended by docs
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  if(esp_wifi_set_mac(WIFI_IF_STA, broadcastMac) != ESP_OK)
  {
    while(true)
    {
      Serial.println("Unable to set mac");
      delay(5000);
    }
  }

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
  if(type == WIFI_PKT_MGMT)
  {
    wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
    int8_t packetRssi = rssiInfo->rx_ctrl.rssi;
    listener.promiscuousRecv(packetRssi);
  }
}


void importConfig(File file, char *param, int maxSize, char buf[])
{
  file.seek(0);
  int i = 0;
  int addToString = 0;

  //Make sure the parameter exists in the file
  if(file.find(param))
  {
    Serial.printf("%s found. Pointer postion %i\n", param, file.position());
  }
  else{
    Serial.printf("[Warning] %s not found.\n", param);
    return;
  }

  int read = 0;
  //Read the value into given buffer
  while(file.available() && file.peek() != '\n' && i < maxSize)
  {
    if(read == 0)
    {
      //Skip any white space between the parameter and the actual value
      if(file.peek() != ' ')
      {
        read = 1;
      }
      else{
        file.read();
      }
    }

    if(read == 1)
    {
      buf[i] = file.read();
      i++;
    }
  }
  buf[i] = '\0';

}


void openConfig()
{
  if(!SPIFFS.begin(true))
  {
    Serial.println("[Error] Unable to start SPIFFS");
    return;
  }

  char lmkConf[] = "lmk:", pmkConf[] = "pmk:", usernameConf[] = "username:";

  File file = SPIFFS.open("/board.cfg");

  if(!file || file.isDirectory())
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  importConfig(file, lmkConf, 16, lmk);
  importConfig(file, pmkConf, 16, pmk);
  importConfig(file, usernameConf, 32, userName);


  file.close();
}


void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);
  xMutex = xSemaphoreCreateMutex();
  openConfig();

  //Set username and check size
  if(listener.setUserName(userName, sizeof(userName)) != 0)
  {
    ////Serial.println("[ERROR] UserName too large");
    return;
  }

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

  //Set default deisplay
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.clearDisplay(); 
  delay(100);
  display.println("Waiting for communication...");
  display.display();
#endif

  init_wifi();

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_pmk((uint8_t*)pmk); //Set primary master key for encryption
  addPeerInfo();

  //Tasks to run on second core
  //When creating these functions, make sure they have some sort of return or vTaskDelete90, even if they are void
  //Otherwise the program will crash

  //Way too much memory allocated to this
  xTaskCreatePinnedToCore(
    handleDisplay
    , "Print peers"
    , 32768
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


//Remove peers if they haven't been seen in more than 10 seconds
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
            if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
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


//Display logos if it has been at least 25 seconds since they were last displayed
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

      //Thread must share nicely with other functions. Otherwise it will crash
      if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
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
          Serial.print("Heap memory: ");
          Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
          Serial.println("Peer loop: ");
          for(int i = 0; i < peers-5; i++)
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
