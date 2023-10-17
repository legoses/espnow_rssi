#ifndef DISPLAYINFO_H
#define DISPLAYINFO_H

#include <peerinfo.h>


class DisplayInfo : public PeerInfo
{
    int8_t sortedRssi[ORDERED_LIST_LEN];
    //char sortedUserNameList[ORDERED_LIST_LEN][32];
    char **sortedUserNameList;

    public:
        DisplayInfo();
        void updatePeers();
        void sortPeers();
        int8_t *getRssi();
        char **getUserName();
};

#endif