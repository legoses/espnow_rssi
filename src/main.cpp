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


char macAddr[][13] = {
  {"c8c9a361cfea"},
  {"F8A38F1D95CF"}
};

//Hold mac address, username, rssi, and time since last seen
char peerDisplayInfo[20][4];

int delayVal = 10;

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


void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  //copy data to message struct
  delayVal = 100;
  memcpy(&recvMessage, incomingData, sizeof(recvMessage)-8); // -8 is so that it does not overwrite the rssi variable, which is 8 bits long
  delayVal = 0;
  
  //Display current connections
  display.clearDisplay();
  display.setCursor(0,10);
  display.printf("%s:    %i\n\r", recvMessage.name, recvMessage.rssi);
  display.display();
}


void promiscuousRecv(void *buf, wifi_promiscuous_pkt_type_t type)
{
  //Get connection info
  delay(delayVal);
  wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
  recvMessage.rssi = rssiInfo->rx_ctrl.rssi;
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

void setup() {
  Serial.begin(115200);


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
