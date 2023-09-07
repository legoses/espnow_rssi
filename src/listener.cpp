#include <listener.h>
#include <Arduino.h>


void PeerListener::promiscuousRecv(int8_t packetRssi)
{
    //Get connection info
    if(xMutex != NULL)
    {
        if(xSemaphoreTake(xMutex, getMaxDelay()) == pdTRUE)
        {
            this->tempRssi = packetRssi;
        }
        xSemaphoreGive(xMutex);
    }
}


void PeerListener::updatePeers()
{
    for(int i = 0; i < getNumCurPeer(); i++)
    {
        this->sortedRssi[i] = this->rssi[i];
        memcpy(sortedUserNameList[i], userNameList[i], 32);
        Serial.printf("Loaded user name: %s\n", sortedUserNameList[i]);
    }
}


int8_t PeerListener::getRssi(int i)
{
    Serial.printf("Returning %i\n", i);
    return this->sortedRssi[i];
}

char *PeerListener::getUserName(int i)
{
    for(int i = 0; i < 10; i++)
    {
        Serial.printf("%s is at %i in sorted. %s is at %i in non sorted \n", sortedUserNameList[i], i, userNameList[i], i);
    }
    return this->sortedUserNameList[i];
}


void PeerListener::sortPeers()
{
    int peers = getNumCurPeer();
    if(peers > 1)
    {
        bool swap = false;
        for(int i = 0; i < peers; i++)
        {
            for(int j = 0; j < peers - i; j++)
            {
                int8_t rssiPlaceHolder;
                char namePlaceHolder[32];

                if((this->sortedRssi[i] < this->sortedRssi[i+1]))
                {
                    rssiPlaceHolder = this->sortedRssi[i];
                    memcpy(namePlaceHolder, this->sortedUserNameList[i], 31);

                    this->sortedRssi[i] = this->sortedRssi[i+1];
                    memcpy(this->sortedUserNameList[i], this->sortedUserNameList[i+1], 31);

                    this->sortedRssi[i+1] = rssiPlaceHolder;
                    memcpy(this->sortedUserNameList[i+1], namePlaceHolder, 31);
                    swap = true;
                }

                if(swap == false)
                {
                    break;
                }
            }
        }
    }
}


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
            //int8_t tempRssi = recvMessage.rssi;

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
                //update peers
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
                    Serial.print("new peer ");
                    Serial.println(i);
                    copyMac(mac, i);
                    rssi[i] = this->tempRssi;
                    memcpy(userNameList[i-1], (char*)incomingData, 31);
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