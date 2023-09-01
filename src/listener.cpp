#include <listener.h>
#include <Arduino.h>


PeerInfo::PeerInfo()
{
    
    this->numCurPeer = 0;
    this->xMutex = xSemaphoreCreateMutex();
}


void PeerInfo::setNumCurPeer(int num)
{
    this->numCurPeer += num;
}


int PeerInfo::getNumCurPeer()
{
    return this->numCurPeer;
}


void PeerListener::promiscuousRecv(void *buf, wifi_promiscuous_pkt_type_t type)
{
    //Get connection info
    if(PeerInfo::xMutex != NULL)
    {
        if(xSemaphoreTake(PeerInfo::xMutex, MAX_DELAY) == pdTRUE)
        {
            wifi_promiscuous_pkt_t *rssiInfo = (wifi_promiscuous_pkt_t *)buf;
            //recvMessage.rssi = rssiInfo->rx_ctrl.rssi;
            this->tempRssi = rssiInfo->rx_ctrl.rssi;
        }
        xSemaphoreGive(PeerInfo::xMutex);
    }
}


void PeerListener::copyMac(const uint8_t *mac, int j)
{
  for(int i = 0; i < 4; i++)
  {
    incomingMac[j][i] = mac[i];
  }
}



void PeerListener::OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
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
    if(PeerInfo::xMutex != NULL)
    {
        if(xSemaphoreTake(PeerInfo::xMutex, MAX_DELAY) == pdTRUE)
        {
            //int8_t tempRssi = recvMessage.rssi;

            for(int i = 0; i < 4; i++)
            {
                Serial.print(mac[i]);
            }

            Serial.println();
            Serial.println((char*)incomingData);

            for(int i = 0; i <  ORDERED_LIST_LEN; i++)
            {
                //Init first peer
                if(PeerInfo::getNumCurPeer() == 0)
                {
                    copyMac(mac, 0);
                    rssi[0] = this->tempRssi;
                    memcpy(PeerInfo::userNameList[0], incomingData, 31);
                    lastSeen[0] = millis();
                    PeerInfo::setNumCurPeer(1);
                    break;
                }
                //update peers
                else if(memcmp(mac, incomingMac[i], 4) == 0)
                {
                    Serial.print("updating ");
                    Serial.println(i);
                    PeerInfo::rssi[i] = this->tempRssi;
                    memcpy(PeerInfo::userNameList[i], incomingData, 31);
                    PeerInfo::lastSeen[i] = millis();
                    break;
                }
                //add new peer
                else if(i > PeerInfo::getNumCurPeer() - 1)
                {
                    Serial.print("new peer ");
                    Serial.println(i);
                    copyMac(mac, i);
                    PeerInfo::rssi[i] = this->tempRssi;
                    memcpy(PeerInfo::userNameList[i], incomingData, 31);
                    PeerInfo::lastSeen[i] = millis();
                    PeerInfo::setNumCurPeer(1);
                    break;
                }
            }
        }
        xSemaphoreGive(xMutex);
    }
}

/*
void PeerListener::begin(char macAddr[][13], int peers)
{
    
    //Register funciton to be called every time an esp-now packet is revieved
    esp_now_register_recv_cb(PeerListener::OnDataRecv);
    //esp_now_register_send_cb(OnDataSent);

    //Set adaptor to promiscuous mode in order to recieve connection info, and register callback function
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(*PeerListener::promiscuousRecv);
}

class PeerListener : public PeerInfo
{
    
    SemaphoreHandle_t xMutex = NULL;

    void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
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
            if(xSemaphoreTake(xMutex, MAX_DELAY) == pdTRUE)
            {
                int8_t tempRssi = recvMessage.rssi;

                for(int i = 0; i < 4; i++)
                {
                    Serial.print(mac[i]);
                }

                Serial.println();
                Serial.println((char*)incomingData);

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

    public:
        void begin(char macAddr[][13], int peers)
        {
            //Register funciton to be called every time an esp-now packet is revieved
            esp_now_register_recv_cb(OnDataRecv);
            esp_now_register_send_cb(OnDataSent);

            //Set adaptor to promiscuous mode in order to recieve connection info, and register callback function
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_promiscuous_rx_cb(promiscuousRecv);
        }
}
*/