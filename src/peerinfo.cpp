//The main purpose of this file is to keep track of the current numbers of peers

#include <peerinfo.h>

int PeerInfo::numCurPeer = 0;
int8_t PeerInfo::rssi[ORDERED_LIST_LEN];
uint8_t PeerInfo::incomingMac[ORDERED_LIST_LEN][16];
char PeerInfo::userNameList[ORDERED_LIST_LEN][32];
long PeerInfo::lastSeen[ORDERED_LIST_LEN];

PeerInfo::PeerInfo()
{
    xMutex = xSemaphoreCreateMutex();
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


void PeerInfo::copyIdentifier(uint8_t identifier[], int j)
{
  for(int i = 0; i < 16; i++)
  {
    this->incomingMac[j][i] = identifier[i];
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
