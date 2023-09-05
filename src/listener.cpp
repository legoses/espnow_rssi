#include <listener.h>
#include <Arduino.h>

int PeerInfo::numCurPeer = 0;
int8_t PeerInfo::rssi[ORDERED_LIST_LEN];
uint8_t PeerInfo::incomingMac[ORDERED_LIST_LEN][5];
char PeerInfo::userNameList[ORDERED_LIST_LEN][32];
long PeerInfo::lastSeen[ORDERED_LIST_LEN];

PeerInfo::PeerInfo()
{
    this->xMutex = xSemaphoreCreateMutex();
}

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


void PeerInfo::addPeer()
{
    this->numCurPeer += 1;
}


void PeerInfo::removePeer()
{
    this->numCurPeer -= 1;
}


int PeerInfo::getNumCurPeer()
{
    return this->numCurPeer;
}


void PeerInfo::copyMac(const uint8_t *mac, int j)
{
  for(int i = 0; i < 4; i++)
  {
    this->incomingMac[j][i] = mac[i];
  }
}

const int PeerInfo::getMaxDelay()
{
    return this->MAX_DELAY;
}


const int PeerInfo::getOrderedListLen()
{
    return this->ORDERED_LIST_LEN;
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
                    memcpy(userNameList[i], incomingData, 31);
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