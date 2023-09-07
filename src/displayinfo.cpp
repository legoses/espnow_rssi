#include <displayinfo.h>

void DisplayInfo::updatePeers()
{
    for(int i = 0; i < getNumCurPeer(); i++)
    {
        this->sortedRssi[i] = this->rssi[i];
        memcpy(sortedUserNameList[i], userNameList[i], 32);
        Serial.printf("Loaded user name: %s\n", sortedUserNameList[i]);
    }
}


int8_t DisplayInfo::getRssi(int i)
{
    Serial.printf("Returning %i\n", i);
    return this->sortedRssi[i];
}


char *DisplayInfo::getUserName(int i)
{
    for(int i = 0; i < 10; i++)
    {
        Serial.printf("%s is at %i in sorted. %s is at %i in non sorted \n", sortedUserNameList[i], i, userNameList[i], i);
    }
    return this->sortedUserNameList[i];
}


void DisplayInfo::sortPeers()
{
    int peers = getNumCurPeer();
    if(peers > 1)
    {
        bool swap = false;
        for(int i = 0; i < peers; i++)
        {
            for(int j = 0; j < peers - i; j++)
            {
                int8_t rssiPlaceHolder;
                char namePlaceHolder[32];

                if((this->sortedRssi[j] < this->sortedRssi[j+1]))
                {
                    rssiPlaceHolder = this->sortedRssi[j];
                    memcpy(namePlaceHolder, this->sortedUserNameList[j], 31);

                    this->sortedRssi[j] = this->sortedRssi[j+1];
                    memcpy(this->sortedUserNameList[j], this->sortedUserNameList[j+1], 31);

                    this->sortedRssi[j+1] = rssiPlaceHolder;
                    memcpy(this->sortedUserNameList[j+1], namePlaceHolder, 31);
                    swap = true;
                }

                if(swap == false)
                {
                    break;
                }
            }
        }
    }
}