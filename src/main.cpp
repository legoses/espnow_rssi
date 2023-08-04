/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/


#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "esp_now.h"
#include "esp_wifi.h"


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SDA 39
#define I2C_SCL 38

//Create mutex to lock objects and prevent race conditions
#define USE_MUTEX
#define ARDUINO_RUNNING_CORE 1

SemaphoreHandle_t xMutex = NULL;


char macAddr[][13] = {
  {"c8c9a361cfea"},
  {"F8A38F1D95CF"}, //Fake mac for testing. Remove later
  {"f412fa682eac"}
};

struct connectionInfo
{
  uint8_t mac[5];
  int8_t rssi;
  char userName[32];
  long lastSeen;

  struct connectionInfo *next;
  struct connectionInfo *prev;
};

connectionInfo *headNode = NULL;

const int MAX_DELAY = 100;

//Username you want to show up on other displays
char userName[] = "userName";

int macNum = sizeof(macAddr) / sizeof(macAddr[0]);

//The mac address of the device you want to communicate with
uint8_t broadcastAddress[20][6];

//Set up display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


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


void initNode(const uint8_t *mac, const uint8_t *incomingData, connectionInfo *node)
{
  connectionInfo *new_node = new connectionInfo();

  for(int i = 0; i < 4; i++)
  {
    new_node->mac[i] = mac[i];
  }
  new_node->lastSeen = millis();
  node->next = new_node;
  new_node->prev = node;
}


void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  if(xMutex != NULL)
  {
    if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
    {
      int8_t rssi = recvMessage.rssi;
      connectionInfo *temp = headNode;
      int userFound = 0;

      //Init the first node in linked list
      if(headNode == NULL)
      {
        headNode = new connectionInfo();
        for(int i = 0; i < 4; i++)
        {
          //copy mac to new node
          headNode->mac[i] = mac[i];
        }
        //set current time
        headNode->lastSeen = millis();
      }
      

      while(temp != NULL)
      {
        Serial.println(memcmp(mac, temp->mac, 4));
        if(memcmp(mac, temp->mac, 4) == 0)
        {
          userFound = 1;
          temp->rssi = rssi;
          memcpy(temp->userName, incomingData, 31);
          temp->lastSeen = millis();
        }
        else if(temp->next == NULL && userFound == 0)
        {
          initNode(mac, incomingData, temp);
        }

        temp = temp->next;
      }
    }
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
  peerInfo.encrypt = false;
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
  esp_now_add_peer(&peerInfo);
  }
}


void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);

#ifdef USE_MUTEX
  xMutex = xSemaphoreCreateMutex();
#endif



  //Setup i2c connection 
  Wire.begin(I2C_SDA, I2C_SCL);

  //Init display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("Display allocation failed");
  } 

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.clearDisplay(); 
  delay(100);
  display.display();

  init_wifi();

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  addPeerInfo();

  //Copy username to sendMessate struct 
  strcpy(sendMessage.name, userName);

  display.println("Waiting for communication...");
  display.display();

  //Tasks to run on second core
  //When creating these functions, make sure they have some sort of return or vTaskDelete90, even if they are void
  //Otherwise the program will crash
  xTaskCreatePinnedToCore(
    handleDisplay
    , "Print peers"
    , 2048
    , NULL
    , 1
    , NULL
    , ARDUINO_RUNNING_CORE
  );

  xTaskCreatePinnedToCore(
    checkForDeadPeers
    , "Print peers"
    , 2048
    , NULL
    , 2
    , NULL
    , ARDUINO_RUNNING_CORE
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


//Remove the first node in the linked list and deallocate memory
void removeHeadNode()
{
  if(xMutex != NULL)
  {
    if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
    {
      connectionInfo *node = headNode;
      if(headNode->next != NULL)
      {
        headNode = headNode->next;
        headNode->prev = NULL;
        free(node);
      }
      
    }
    
    xSemaphoreGive(xMutex);
  }
}


//Check for inactive peers
void checkForDeadPeers(void *pvParameters)
{
  (void)pvParameters;
  while(true)
  {
    //Do not run right away
    delay(230000);
    connectionInfo *node = headNode;
    
    while(node != NULL)
    {
      if((millis() - node->lastSeen) > 230000 && (node->prev == NULL))
      {
        removeHeadNode();
      }
      //I keep forgetting to add this line, get an infinite loop, and wondering why the program crashes
      node = node->next;
    }
  }
}

//Print out peers to the display
void handleDisplay(void* pvParameters)
{
  (void)pvParameters;
  while(true)
  {
    connectionInfo *node = headNode;

    
    if(xMutex != NULL)
    {
      if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
      {
        display.clearDisplay();
        display.setCursor(0, 10);
        while(node != NULL)
        {
          display.print(node->userName);
          display.print("   ");
          display.println(node->rssi);
          node = node->next;
        }
      display.display();
      }
      xSemaphoreGive(xMutex);
    }
    delay(10000);
  }
  
  return;
}

