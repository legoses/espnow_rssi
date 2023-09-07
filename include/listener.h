#ifndef LISTENER_H
#define LISTENER_H


#include "esp_wifi.h"
#include <peerinfo.h>



class PeerListener : public PeerInfo
{
    int8_t tempRssi;
    int8_t sortedRssi[ORDERED_LIST_LEN];
    char sortedUserNameList[ORDERED_LIST_LEN][32];


    public:
        void begin(char macAddr[][13], int peers);
        int dataRecv(const uint8_t *mac, const uint8_t *incomingData);
        void promiscuousRecv(int8_t packetRssi);
        static void testFunct();
        void updatePeers();
        void sortPeers();
        int8_t getRssi(int i);
        char *getUserName(int i);
};

#endif