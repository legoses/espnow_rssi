#ifndef LISTENER_H
#define LISTENER_H

#include <stdint.h>
#include "esp_wifi.h"

class PeerInfo
{
    static const int MAX_DELAY = 1000;
    static const int ORDERED_LIST_LEN = 25;

    static int numCurPeer;
    

    public:
        PeerInfo();
        static uint8_t incomingMac[ORDERED_LIST_LEN][5];
        static int8_t rssi[ORDERED_LIST_LEN];
        static char userNameList[ORDERED_LIST_LEN][32];
        static long lastSeen[ORDERED_LIST_LEN];
        
        SemaphoreHandle_t xMutex = NULL;

        int getNumCurPeer();
        void addPeer();
        void removePeer();

        void copyMac(const uint8_t *mac, int j);
        const int getMaxDelay();
        const int getOrderedListLen();
        
        

        
    
};


class PeerListener : public PeerInfo
{
    int8_t tempRssi;

    public:
        void begin(char macAddr[][13], int peers);
        int dataRecv(const uint8_t *mac, const uint8_t *incomingData);
        void promiscuousRecv(int8_t packetRssi);
        static void testFunct();
};

#endif