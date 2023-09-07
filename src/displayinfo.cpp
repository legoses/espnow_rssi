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
            for(int j = 0; j < peers - i - 1; j++)
            {
                int8_t rssiPlaceHolder;
                char namePlaceHolder[32];

                if((this->sortedRssi[j] < this->sortedRssi[j+1]))
                {
                    //Serial.printf("Placing rssi in placeholder\n");
                    rssiPlaceHolder = this->sortedRssi[j];
                    //Serial.printf("Placing name in placeholder\n");
                    memcpy(namePlaceHolder, this->sortedUserNameList[j], 31);

                    //Serial.printf("Placing rssi in %i\n", j);
                    this->sortedRssi[j] = this->sortedRssi[j+1];
                    //Serial.printf("Placing name in %i\n", j);
                    memcpy(this->sortedUserNameList[j], this->sortedUserNameList[j+1], 31);

                    //Serial.printf("Placing rssi in %i\n", j+1);
                    this->sortedRssi[j+1] = rssiPlaceHolder;
                    //Serial.printf("Placing name in %i\n", j+1);
                    memcpy(this->sortedUserNameList[j+1], namePlaceHolder, 31);
                    swap = true;
                    if(j+1 == 2)
                    {
                        Serial.printf("Error may be happening at %i\n", i);
                    }
                }

                if(swap == false)
                {
                    break;
                }
            }
        }
    }
}