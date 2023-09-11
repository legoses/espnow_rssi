#include <listener.h>
#include <Arduino.h>


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
        Serial.println("removing");
        Serial.println(i);
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
    char buf[18];
    
    Serial.println("recv Data");
    Serial.print("Recieved MAC: ");
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x::%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    for(int i = 0; i < 4; i++)
    {
        Serial.print(mac[i]);
        Serial.print(" ");
    }
    //Debug
    Serial.println();
    Serial.println(buf);
    Serial.print("Incoming Data: ");
    Serial.println(*incomingData);
    Serial.println();
    Serial.println();
    if(xMutex != NULL)
    {
        if(xSemaphoreTake(xMutex, getMaxDelay()) == pdTRUE)
        {
            Serial.print("Raw MAC: ");
            for(int i = 0; i < 4; i++)
            {
                Serial.print(mac[i]);
            }

            Serial.println();
            Serial.println((char*)incomingData);

            for(int i = 0; i < getOrderedListLen(); i++)
            {
                //Init first peer
                if(getNumCurPeer() == 0)
                {
                    copyMac(mac, 0);
                    rssi[0] = this->tempRssi;
                    memcpy(userNameList[0], incomingData, 31);
                    lastSeen[0] = millis();
                    addPeer();
                    xSemaphoreGive(xMutex);
                    break;
                }
                //update an existing peer
                else if(memcmp(mac, incomingMac[i], 4) == 0)
                {
                    Serial.print("updating ");
                    Serial.println(i);
                    rssi[i] = this->tempRssi;
                    memcpy(userNameList[i], incomingData, 31);
                    lastSeen[i] = millis();
                    break;
                }
                //add new peer
                else if(i > getNumCurPeer() - 1)
                {
                    Serial.print("Init new peer ");
                    Serial.println(i);
                    copyMac(mac, i);
                    rssi[i] = this->tempRssi;
                    memcpy(userNameList[i], (char*)incomingData, 31);
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