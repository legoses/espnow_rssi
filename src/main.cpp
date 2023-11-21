/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

/*
  TODO:
  Implement config file option for username and choice of encryption
*/

#include <Arduino.h>
#include <badge_images.h>
#include <display_username.h>
#include <listener.h>
#include <displayinfo.h>
#include <esp_heap_caps.h>
#include <SPIFFS.h>

//#define heltec_wifi_kit_32_V3
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
# define I2C_SDA 8
# define I2C_SCL 12
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

/* USER CONFIG */
char *userName;

bool encryptESPNOW;
/* END OF USER CONFIG */


/*
  TODO:
  Make sure program does not crash if there are more than 25 peers
  Make logos optional
*/

const int SCREEN_REFRESH = 2500;

PeerListener listener;
DisplayInfo displayInfo;

//Hold mac address once parsed
uint8_t broadcastAddress[20][6];

int numCurPeer = 0;

long timeSinceLastLogo = 0;

esp_now_peer_info_t peerInfo;

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


char *getConfigVal(const char *val, int size, int critical) {
  File file = SPIFFS.open("/board.cfg");
  if(!file) {
    Serial.println("Failed to open board.cfg");
    while(1);
  }

  char ch;
  char *buf = (char*)malloc(sizeof(char) * size);
  int count = 0;

  if(file.find(val)) {
    Serial.print("Cursor position: ");
    Serial.println(file.position());

    while(ch != '\n' && file.available() && count < size) {
      ch = file.read();
      //Serial.println(ch);
      if(ch != ' ') {
        buf[count] = ch;
        count++;
      }
    }

    //If size variable is larger than the actual string found, a line break is included at the end of the string\
    //This checks for and overwrites with null character to mark the end of the string
    if(count > 0) {
      if(buf[count-1] == '\n') {
        buf[count-1] = '\0';
      }
      else {
        buf[count] = '\0';
      }
    }
    
  }
  else if(critical == 1) {
    Serial.printf("[ERROR] Config option %s not found!\n", val);
    while(1);
  }

  file.close();
  return buf;
}


void setEncryptStatus() {
  char *status = getConfigVal("encrypt:", 10, 0);
  const char *checkStatus = "true";

  if(strcmp(status, checkStatus) == 0) {
    encryptESPNOW = true;
    Serial.println("Encryption enabled");
  }
  else {
    encryptESPNOW = false;
    Serial.println("Encryption disabled");
  }
}


//Load espnow peers
void addPeerInfo(int critical)
{
  //Set channel
  peerInfo.channel = 0;
  uint8_t macAddr[] = {0xF4, 0x12, 0xFA, 0x66, 0xEB, 0x00};
  memcpy(peerInfo.peer_addr, macAddr, 6);

  static const char *lmk = getConfigVal("lmk:", 17, critical);
  Serial.print("lmk: ");
  Serial.println(lmk);
  Serial.println(lmk[0]);

  for(int i = 0; i < 16; i++)
  {
    Serial.print("lmk: ");
    Serial.println(lmk[i]);
    peerInfo.lmk[i] = lmk[i];
  }
  
  peerInfo.encrypt = encryptESPNOW;

  if(esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
  }
}

//Display logos on screen

void displayLogos()
{
#ifdef heltec_wifi_kit_32_V3
  int screenEdgeBuf = ((img_buffer / 2) * -1) + 10;
  int pimaLogoDelay = 0;

  //Have pima logo move across the screen
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
#endif
}


//Handle espnow packets
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.println("DATA RECIEVED");
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



void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);
  xMutex = xSemaphoreCreateMutex();

  //Init filesystem
  if(!SPIFFS.begin(true)) {
    Serial.println("Unable to init SPIFFS.");
    return;
  }

  userName = (char*)malloc(sizeof(char)*32);
  userName = getConfigVal("username:", 32, 1);

  //Enable or disable encryption
  setEncryptStatus();

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

  int critical;
  if(encryptESPNOW == true) {
    critical = 1;
  }
  else {
    critical = 0;
  }

  addPeerInfo(critical);
  
  static const char *pmk = getConfigVal("pmk:", 17, critical);
  Serial.print("pmk: ");
  Serial.println(pmk);
  Serial.println(esp_now_set_pmk((uint8_t*)pmk)); //Set primary master key for encryption

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
  //Serial.println("Encrypted Peers:");
  //Serial.println(esp_now_peer_num->encrypt_num);
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
    //clearScreen();

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
      //Play logos if a certain period of time has passed
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
        //displayInfo.sortPeers();

        //These arrays hold the sorted information that will be displayed on the screen
        int8_t *sortRssi = displayInfo.getRssi();
        char **sortUserName = displayInfo.getUserName();

//Display peers on heltec
#ifdef heltec_wifi_kit_32_V3
        Heltec.display->clear();
        //displayUsername(userName);
        //Heltec.display->setColor(BLACK);
        //Heltec.display->fillRect(0, 0, 100, 64);
        //Heltec.display->setColor(WHITE);
        
        //dispBat();
        int yCursorPos = 5;
        char tmpRssi[6];
        Serial.print("PEERS: ");
        Serial.println(peers);

        //Display this message if there are no other peers
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
              //Fix this to return entire array instead of calling one username at a time
              //char *tempUserName = displayInfo.getUserName(j);
              //Serial.printf("Username: %s\n", tempUserName);
              snprintf(tmpRssi, 5, "%d", sortRssi[j]);

              char placeNum[10];
              snprintf(placeNum, 10, "%i. ", j+1);
              ////Serial.println(placeNum);
              
              Heltec.display->drawString(0, yCursorPos, placeNum);
              Heltec.display->drawString(15, yCursorPos, sortUserName[j]);
              Heltec.display->drawString(100, yCursorPos, tmpRssi);
              yCursorPos+=10;
              
            }
            Heltec.display->display();
            delay(1500);
            yCursorPos = 5;

            //Update and sort peer information every three iterations
            if(i % 3 == 0)
            {
              displayInfo.updatePeers();
            }
          }
          Serial.println("reverse");
          //Display peers is reverse order
          
          for(int i = peers; i > 5; i--)
          {
            
            Heltec.display->clear();
            for(int j = i-5; j < i; j++)
            {
              Serial.println(j);
              //char *tempUserName = displayInfo.getUserName(j);
              //Serial.printf("Username: %s\n", tempUserName);
              snprintf(tmpRssi, 5, "%d", sortRssi[j]);

              char placeNum[10];
              snprintf(placeNum, 10, "%i. ", j+1);
              ////Serial.println(placeNum);
              
              Heltec.display->drawString(0, yCursorPos, placeNum);
              Heltec.display->drawString(15, yCursorPos, sortUserName[j]);
              Heltec.display->drawString(100, yCursorPos, tmpRssi);
              yCursorPos+=10;
              
            }
            Heltec.display->display();
            delay(1500);
            yCursorPos = 5;

            //Update and sort peer information every three iterations
            if(i % 3 == 0)
            {
              displayInfo.updatePeers();
            }
          }
        }
        else
        {
          //Display all usernames on the screen at once
          Heltec.display->clear();
          for(int i = 0; i < peers; i++)
          {
            //char *tempUserName = displayInfo.getUserName(i);
            //Serial.printf("Username: %s\n", tempUserName);
            snprintf(tmpRssi, 5, "%d", sortRssi[i]);

            char placeNum[10];
            snprintf(placeNum, 10, "%i. ", i+1);
            ////Serial.println(placeNum);

            Heltec.display->drawString(0, yCursorPos, placeNum);
            Heltec.display->drawString(15, yCursorPos, sortUserName[i]);
            Heltec.display->drawString(100, yCursorPos, tmpRssi);
            yCursorPos+=10;
          }
        }
        
        Heltec.display->display();

//Display peers on ssd1306
#else
        if(peers == 0) {
          display.println("You are alone");
        }
        else if(peers > 5)
        {
          Serial.print("Heap memory: ");
          Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
          Serial.println("Peer loop: ");
          for(int i = 0; i < peers-5; i++)
          {
            display.clearDisplay();
            //Serial.println(i);
            for(int j = i; j < i+5; j++)
            {
              display.printf("%i %s %i\n", j+1, sortUserName[j], sortRssi[j]);
            }
            display.display();
            delay(1500);

            //Update and sort peer information every three iterations
            if(i % 3 == 0)
            {
              displayInfo.updatePeers();
            }
          }
          Serial.println("reverse");
          //Display peers is reverse order
          
          for(int i = peers; i > 5; i--)
          {
            display.clearDisplay();
            for(int j = i-5; j < i; j++)
            {
              display.printf("%i. %s %i\n", j+1, sortUserName[j], sortRssi[j]);
            }
            display.display();
            delay(1500);

            //Update and sort peer information every three iterations
            if(i % 3 == 0)
            {
              displayInfo.updatePeers();
            }
          }
        }
        else
        {
          //Display all usernames on the screen at once
          display.clearDisplay();
          for(int i = 0; i < peers; i++)
          {
            display.printf("%i %s %i\n", i+1, sortUserName[i], sortRssi[i]);
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
