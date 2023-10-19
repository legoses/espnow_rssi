#include <listener.h>
#include <Arduino.h>
#include <bootloader_random.h>
#include "esp_now.h"


int PeerListener::setUserName(char *userName, int size)
{
    if(size <= 32)
    {
        strcpy(sendInfo.userName, userName);
    }
    else
    {
        return 1;
    }
    return 0;
}


void PeerListener::send_esp()
{
  esp_err_t result = esp_now_send(NULL, (uint8_t*)&sendInfo, sizeof(sendInfo));
  if(result == ESP_OK)
  {
    Serial.println("Sent With Success");
  }
  else 
  {
    Serial.println("Error Sending Data");
  }

}


PeerListener::PeerListener()
{
    bootloader_random_enable();
    //selfIdentifier = esp_random();
    bootloader_fill_random(sendInfo.selfIdentifier, 16);
    bootloader_random_disable();
}


void PeerListener::promiscuousRecv(int8_t packetRssi)
{
    //Since arduino implementation of espnow does not return rssi, the network interface is put into
    //promiscuous mode in order to get rssi from all packets
    if(xMutex != NULL)
    {
        if(xSemaphoreTake(xMutex, getMaxDelay()) == pdTRUE)
        {
            this->tempRssi = packetRssi;
        }
        xSemaphoreGive(xMutex);
    }
}


void PeerListener::removeDeadPeer(int item)
{
  //Remove peers that have not been seen in the last 10 seconds and all arrays
    for(int i = item; i < getOrderedListLen()-1; i++)
    {
        //Serial.println("removing");
        //Serial.println(i);
        memcpy(incomingMac[i], incomingMac[i+1], 4);
        rssi[i] = rssi[i+1];
        memcpy(userNameList[i], userNameList[i+1], 31);
        lastSeen[i] = lastSeen[i+1];
    }
}


long PeerListener::getTimeLastSeen(int i)
{
    if(i < getNumCurPeer())
    {
        return lastSeen[i];
    }
    return 0;
}

//Handle espnow packets
int PeerListener::dataRecv(const uint8_t *mac, const uint8_t *incomingData)
{
    memcpy(&recvInfo, incomingData, sizeof(recvInfo));
    char buf[18];
    
    //Debug
    if(xMutex != NULL)
    {
        if(xSemaphoreTake(xMutex, getMaxDelay()) == pdTRUE)
        {
            for(int i = 0; i < getOrderedListLen(); i++)
            {
                //Init first peer
                if(getNumCurPeer() == 0)
                {
                    copyIdentifier(recvInfo.selfIdentifier, 0);
                    rssi[0] = this->tempRssi;
                    memcpy(userNameList[0], recvInfo.userName, 31);
                    lastSeen[0] = millis();
                    addPeer();
                    xSemaphoreGive(xMutex);
                    break;
                }
                //update an existing peer
                else if(memcmp(recvInfo.selfIdentifier, incomingMac[i], 4) == 0)
                {
                    //Serial.print("updating ");
                    //Serial.println(i);
                    rssi[i] = this->tempRssi;
                    memcpy(userNameList[i], recvInfo.userName, 31);
                    lastSeen[i] = millis();
                    break;
                }
                //add new peer
                else if(i > getNumCurPeer() - 1)
                {
                    //Serial.print("Init new peer ");
                    //Serial.println(i);
                    copyIdentifier(recvInfo.selfIdentifier, i);
                    rssi[i] = this->tempRssi;
                    memcpy(userNameList[i], recvInfo.userName, 31);
                    lastSeen[i] = millis();
                    addPeer();
                    xSemaphoreGive(xMutex);
                    break;
                }
            }
        }
        xSemaphoreGive(xMutex);
    }
    return 0;
}