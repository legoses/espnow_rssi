#ifndef PEERINFO_H
#define PEERINFO_H
#include <stdint.h>
#include <Arduino.h>

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

#endif