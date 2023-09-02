#ifndef LISTENER_H
#define LISTENER_H

#include <stdint.h>
#include "esp_wifi.h"

class PeerInfo
{
    
    int numCurPeer;
    


    public:
    PeerInfo();

    SemaphoreHandle_t xMutex = NULL;

    int getNumCurPeer();
    void setNumCurPeer(int num);
    
    static const int MAX_DELAY = 1000;
    static const int ORDERED_LIST_LEN = 25;

    uint8_t incomingMac[ORDERED_LIST_LEN][5];
    int8_t rssi[ORDERED_LIST_LEN];
    char userNameList[ORDERED_LIST_LEN][32];
    long lastSeen[ORDERED_LIST_LEN];
    
};


class PeerListener : public PeerInfo
{
    void copyMac(const uint8_t *mac, int j);
    int8_t tempRssi;

    public:
        void begin(char macAddr[][13], int peers);
        void dataRecv(const uint8_t *mac, const uint8_t *incomingData);
        void promiscuousRecv(int8_t packetRssi);
};

#endif