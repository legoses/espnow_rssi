#ifndef LISTENER_H
#define LISTENER_H


#include "esp_wifi.h"
#include <peerinfo.h>



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