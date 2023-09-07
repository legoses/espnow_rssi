/*
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
  received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

/*
  TODO:
  Fix dataRecv adding new peers where it wants to instead of where I am telling it to
  Update remove peer function
*/

#include <Arduino.h>
#include <badge_images.h>
#include <display_username.h>
#include <listener.h>

#define heltec_wifi_kit_32_V3
#define USE_MUTEX
#define ARDUINO_RUNNING_CORE 1

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

PeerListener listener;

//Change to true to enable encryption
bool encryptESPNOW = false;

//Each of these must contain a 16 byte string
static const char *pmk = "<PMK here>";
static const char *lmk = "<LMK here>";

//Username you want to show up on other displays
char userName[] = "legoses";

//Insert the MAC addresses of the boards this board will be communicating with
//Insert mac address ad string, removing colons
char macAddr[][13] = {
  // {"8E5CDAEE1697"}, Example mac
  // {"0504A4C587AF"}  Example mac
  //{"f412fa815118"}, //Eric's mac
  //{"F412FA66EB00"}, //Kyle's mac
  //{"f412fa66e9ec"} // Paul's mac
  {"7CDFA1E403AC"}, //non heltec
};

const int SCREEN_REFRESH = 2500;

//Hold mac address once parsed
uint8_t broadcastAddress[20][6];

int numCurPeer = 0;

int macNum = sizeof(macAddr) / sizeof(macAddr[0]);
long timeSinceLastLogo = 0;



//Hold info to send and recieve data
typedef struct struct_message {
  int8_t rssi;
  char userName[32];
} struct_message;

//struct_message sendMessage;
struct_message recvMessage;


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
  //set default wifi config as recommended by docs
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);

  esp_wifi_start();
}


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
  delay(2500-pimaLogoDelay);
  Heltec.display->clear();
  Heltec.display->drawXbm(screenEdgeBuf, 0, 71, 62, range_logo);
  Heltec.display->display();
  delay(2500);
}


void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.println("DATA RECIEVED");
  listener.dataRecv(mac, incomingData);
}

void promiscuousRecv(void *buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
  int8_t packetRssi = rssiInfo->rx_ctrl.rssi;
  listener.promiscuousRecv(packetRssi);
}


void handleDisplay(void *pvParameters);
void checkForDeadPeers(void *pvParameters);


void setup() {
  Serial.begin(115200);
  xMutex = xSemaphoreCreateMutex();

  //Init display
#ifdef heltec_wifi_kit_32_V3
  Heltec.begin(true, false, false);
  Heltec.display->init();
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
 
  displayUsername(userName);
  
  displayLogos();

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
  //memcpy(sendMessage.userName, userName, 32);
  Serial.println(xPortGetCoreID());

  init_wifi();

  if(esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
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

  PeerListener listen;


  //Register funciton to be called every time an esp-now packet is revieved
  if(esp_now_register_recv_cb(onDataRecv) != ESP_OK)
  {
    Serial.println("ERRORERRORERROERROORERRORERROROERRORO");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  //Set adaptor to promiscuous mode in order to recieve connection info, and register callback function
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuousRecv);

}


void loop() {
  //Setting first value to NULL will send data to all registered peers
  //esp_err_t result = esp_now_send(NULL, (uint8_t*)&sendMessage, sizeof(sendMessage));
  esp_err_t result = esp_now_send(NULL, (uint8_t*)&userName, sizeof(userName));
  if(result == ESP_OK)
  {
    Serial.println("Sent With Success");
  }
  else 
  {
    Serial.println("Error Sending Data");
  }
  delay(2000);
  Serial.printf("My username: %s\n", userName);
}


//Remove peers who haven't been seen in 10 seconds
void removeItem(int item)
{
  for(int i = item; i < listener.getOrderedListLen()-1; i++)
  {
    Serial.println("removing");
    Serial.println(i);
    memcpy(listener.incomingMac[i], listener.incomingMac[i+1], 4);
    listener.rssi[i] = listener.rssi[i+1];
    memcpy(listener.userNameList[i], listener.userNameList[i+1], 31);
    listener.lastSeen[i] = listener.lastSeen[i+1];
    
  }
}


void checkForDeadPeers(void* pvParameters)
{
  //I don't think this does anything since I don't pass any parameter to this funciton
  //Possibly delete
  (void)pvParameters;
  delay(10000);
  while(true)
  {
    for(int i = 0; i < numCurPeer; i++)
    {
      Serial.println(i);
      if(millis() - listener.lastSeen[i] > 10000)
      {
          if(xMutex != NULL)
          {
            if(xSemaphoreTake(xMutex, listener.getMaxDelay()) == pdTRUE)
            {
              removeItem(i);
              numCurPeer--;
            }
            xSemaphoreGive(xMutex);
        } 
      }
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
  Serial.println(listener.getNumCurPeer());

  for(int i = 0; i < listener.getNumCurPeer(); i++)
  {
    Serial.println(i);
    rssiArr[i] = listener.rssi[i];
    memcpy(sortUserNameList[i], listener.userNameList[i], 31);
  }
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
        //Load list into local array and sort
        Serial.println("Loading arrays...");
        listener.updatePeers();
        Serial.println("Sorting arrays...");
        listener.sortPeers();

//Display peers on heltec
#ifdef heltec_wifi_kit_32_V3
        Heltec.display->clear();
        int yCursorPos = 10;
        char tmpRssi[6];
        
        if(listener.getNumCurPeer() == 0)
        {
          Heltec.display->drawString(0, yCursorPos, "You Are Alone.");
        }
        else
        {
          for(int i = 0; i < listener.getNumCurPeer(); i++)
          {
            char *tempUserName = listener.getUserName(i);
            Serial.printf("Username: %s\n", tempUserName);
            snprintf(tmpRssi, 5, "%d", listener.getRssi(i));
            Heltec.display->drawString(0, yCursorPos, tempUserName);
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
