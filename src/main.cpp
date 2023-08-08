/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <Arduino.h>

#define heltec_wifi_kit_32_V3
#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM 15

#include <Wire.h>
#include "esp_now.h"
#include "esp_wifi.h"

#ifdef heltec_wifi_kit_32_V3
# include "heltec.h"
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



//Create mutex to lock objects and prevent race conditions
#define USE_MUTEX
#define ARDUINO_RUNNING_CORE 1

SemaphoreHandle_t xMutex = NULL;

static const char *pmk = "BDYx9K3HhP7tsZwl";
static const char *lmk = "honHw5sFkCmx74q5";

//Username you want to show up on other displays
char userName[] = "userName";

char macAddr[][13] = {
  //{"c8c9a361cfea"},
  {"F8A38F1D95CF"}, //Fake mac for testing. Remove later
  {"f412fa682eac"},
  {"7CDFA1E403AC"} //devkit
  //{"F412FA66EB00"} //heltec
};

const int ORDERED_LIST_LEN = 25;

uint8_t incomingMac[ORDERED_LIST_LEN][5];
int8_t rssi[ORDERED_LIST_LEN];
char userNameList[ORDERED_LIST_LEN][32];
long lastSeen[ORDERED_LIST_LEN];

int numCurPeer = 0;


const int MAX_DELAY = 1000;

int macNum = sizeof(macAddr) / sizeof(macAddr[0]);

//The mac address of the device you want to communicate with
uint8_t broadcastAddress[20][6];

//Hold info to send and recieve data
typedef struct struct_message {
  char name[32];
  int8_t rssi;
} struct_message;

struct_message sendMessage;
struct_message recvMessage;

esp_now_peer_info_t peerInfo;
int lineCount = 0;
int file = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast packet send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failed");
}


int compareMac(uint8_t knownMac[], uint8_t recvMac[])
{
  for(int i = 0; i < 4; i++)
  {
    if(knownMac[i] != recvMac[i])
    {
      return 1;
    }
  }
  return 0;
}


void copyMac(const uint8_t *mac, int j)
{
  for(int i = 0; i < 4; i++)
  {
    incomingMac[j][i] = mac[i];
  }
}


void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.println("recv Data");
  if(xMutex != NULL)
  {
    if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
    {
      int8_t tempRssi = recvMessage.rssi;

      for(int i = 0; i <  ORDERED_LIST_LEN; i++)
      {
        //Init first peer
        if(numCurPeer == 0)
        {
          copyMac(mac, 0);
          rssi[0] = tempRssi;
          memcpy(userNameList[0], incomingData, 31);
          lastSeen[0] = millis();
          numCurPeer++;
          break;
        }
        //update peers
        else if(memcmp(mac, incomingMac[i], 4) == 0)
        {
          Serial.print("updating ");
          Serial.println(i);
          rssi[i] = tempRssi;
          memcpy(userNameList[i], incomingData, 31);
          lastSeen[i] = millis();
          break;
        }
        //add new peer
        else if(i > numCurPeer-1)
        {
          Serial.print("new peer ");
          Serial.println(i);
          copyMac(mac, i);
          rssi[i] = tempRssi;
          memcpy(userNameList[i], incomingData, 31);
          lastSeen[i] = millis();
          numCurPeer++;
          break;
        }

      }
    }
    xSemaphoreGive(xMutex);
  }
}


void promiscuousRecv(void *buf, wifi_promiscuous_pkt_type_t type)
{
  //Get connection info
  if(xMutex != NULL)
  {
    if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
    {
      wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
      recvMessage.rssi = rssiInfo->rx_ctrl.rssi;
    }
    
    xSemaphoreGive(xMutex);
  }
}


void init_wifi()
{
  //set default wifi config as recommended by docs
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);

  esp_wifi_start();
}


void addPeerInfo()
{
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
  peerInfo.encrypt = true;
  esp_now_add_peer(&peerInfo);
  }
}


void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);

  //Fill arrays with temp data
  for(int i = 0; i < ORDERED_LIST_LEN; i++)
  {
    rssi[i] = 1;
    incomingMac[i][0] = 1;
  }


#ifdef USE_MUTEX
  xMutex = xSemaphoreCreateMutex();
#endif

  //Init display
#ifdef heltec_wifi_kit_32_V3
  Heltec.begin(true, false, false);
  Heltec.display->init();
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, "Waiting for communication...");
  Heltec.display->display();
#else
  //Setup i2c connection 
  Wire.begin(I2C_SDA, I2C_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("Display allocation failed");
  } 

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.clearDisplay(); 
  delay(100);
  display.println("Waiting for communication...");
  display.display();
#endif

  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());

  init_wifi();

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_pmk((uint8_t*)pmk);
  addPeerInfo();

  //Copy username to sendMessate struct 
  strcpy(sendMessage.name, userName);

  

  //Tasks to run on second core
  //When creating these functions, make sure they have some sort of return or vTaskDelete90, even if they are void
  //Otherwise the program will crash

  xTaskCreatePinnedToCore(
    handleDisplay
    , "Print peers"
    , 4096
    , NULL
    , 1
    , NULL
    , 0
  );

  xTaskCreatePinnedToCore(
    checkForDeadPeers
    , "Print peers"
    , 2048
    , NULL
    , 2
    , NULL
    , 0
  );


  //Register funciton to be called every time an esp-now packet is revieved
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  //Set adaptor to promiscuous mode in order to recieve connection info, and register callback function
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRecv);
}

void loop() {
  //Setting first value to NULL will send data to all registered peers
  esp_err_t result = esp_now_send(NULL, (uint8_t*)&sendMessage, sizeof(sendMessage));
  if(result == ESP_OK)
  {
    Serial.println("Sent With Success");
  }
  else 
  {
    Serial.println("Error Sending Data");
  }

  delay(2000);
}


void removeItem(int item)
{
  for(int i = item; i < ORDERED_LIST_LEN-1; i++)
  {
    Serial.println("removing");
    Serial.println(i);
    memcpy(incomingMac[i], incomingMac[i+1], 4);
    rssi[i] = rssi[i+1];
    memcpy(userNameList[i], userNameList[i+1], 31);
    lastSeen[i] = lastSeen[i+1];
    
  }
  rssi[24] = 1;
  incomingMac[24][0] = 1;
}


void checkForDeadPeers(void* pvParameters)
{
  //I don't think this does anything since I don't pass any parameter to this funciton
  //Possibly delete
  (void)pvParameters;
  delay(10000);
  while(true)
  {
    int i = 0;
    while(incomingMac[i][0] != 1)
    {
      Serial.println(i);
      if(millis() - lastSeen[i] > 10000)
      {
          if(xMutex != NULL)
          {
            if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
            {
              removeItem(i);
              numCurPeer--;
            }
            xSemaphoreGive(xMutex);
        } 
      }
      i++;
    }
    delay(10000);
  }
  return;
}

//Move from the global array to a local one for sorting
//Soring global may not be an issue any more because im not using a linked list?
void loadList(int8_t rssiArr[], char sortUserNameList[][32])
{
  Serial.print("peers ");
  Serial.println(numCurPeer);

  for(int i = 0; i < numCurPeer; i++)
  {

    Serial.println(i);
    rssiArr[i] = rssi[i];
    memcpy(sortUserNameList[i], userNameList[i], 31);
  }

}


//Simple bubble sort
void sortList(int8_t rssiArray[], char sortUserNameList[][32])
{
  int8_t rssiPlaceHolder;
  char namePlaceHolder[32];

  if(numCurPeer > 1)
  {
    for(int i = 0; i < numCurPeer; i++)
    {
      if((rssiArray[i] < rssiArray[i+1]))
      {
        rssiPlaceHolder = rssiArray[i];
        memcpy(namePlaceHolder, sortUserNameList[i], 31);

        rssiArray[i] = rssiArray[i+1];
        memcpy(sortUserNameList[i], sortUserNameList[i+1], 31);

        rssiArray[i+1] = rssiPlaceHolder;
        memcpy(sortUserNameList[i+1], namePlaceHolder, 31);
        i = 0;
      }
      Serial.print("Placeholder ");
      Serial.println(rssiPlaceHolder);
    }
  }
}
 

//Print out peers to the display
void handleDisplay(void* pvParameters)
{
  (void)pvParameters;
  int8_t rssiArr[ORDERED_LIST_LEN];
  char sortUserNameList[ORDERED_LIST_LEN][32];

  
  while(true)
  {
    if(xMutex != NULL)
    {
      if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
      {
        //Load list into local array and sort
        loadList(rssiArr, sortUserNameList);
        sortList(rssiArr, sortUserNameList);

//Display peers on heltec
#ifdef heltec_wifi_kit_32_V3

        Heltec.display->clear();
        int yCursorPos = 10;
        char tmpRssi[6];
        Serial.println("display test");
        
        if(numCurPeer == 0)
        {
          Heltec.display->drawString(0, yCursorPos, "You Are Alone.");
        }
        else
        {
          for(int i = 0; i < numCurPeer; i++)
          {
            Serial.println(i);
            snprintf(tmpRssi, 5, "%d", rssiArr[i]);
            Heltec.display->drawString(0, yCursorPos, sortUserNameList[i]);
            Heltec.display->drawString(80, yCursorPos, tmpRssi);
          }
        }
        Heltec.display->display();

//Display peers on cc1101
#else
        display.clearDisplay();
        display.setCursor(0, 10);

        if(numCurPeer == 0)
        {
          display.println("You Are Alone.");
        }
        else
        {
          for(int i = 0; i < numCurPeer; i++)
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
    delay(10000);
  }
  return;
}
