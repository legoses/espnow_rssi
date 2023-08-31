#ifndef LISTENER_H
#define LISTENER_H
const int MAX_DELAY = 1000;

const int ORDERED_LIST_LEN = 25;

class peerInfo
{
    uint8_t incomingMac[ORDERED_LIST_LEN][5];
    int8_t rssi[ORDERED_LIST_LEN];
    char userNameList[ORDERED_LIST_LEN][32];
    long lastSeen[ORDERED_LIST_LEN];
}




#endif