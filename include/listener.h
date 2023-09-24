#ifndef LISTENER_H
#define LISTENER_H


#include "esp_wifi.h"
#include <peerinfo.h>



class PeerListener : public PeerInfo
{
    int8_t tempRssi;

    typedef struct Packet_info {
        char userName[32];
        uint8_t selfIdentifier[16];
    } Packet_Info_t;

    Packet_Info_t sendInfo;
    Packet_Info_t recvInfo;

    public:
        PeerListener();
        void begin(char macAddr[][13], int peers);
        int dataRecv(const uint8_t *mac, const uint8_t *incomingData);
        void promiscuousRecv(int8_t packetRssi);
        void removeDeadPeer(int item);
        long getTimeLastSeen(int i);
        int setUserName(char* userName, int size);
        void nametest();
        void send_esp();
        
        
};

#endif