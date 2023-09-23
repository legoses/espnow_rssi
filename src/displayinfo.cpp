#include <displayinfo.h>

void DisplayInfo::updatePeers()
{
    for(int i = 0; i < getNumCurPeer(); i++)
    {
        this->sortedRssi[i] = this->rssi[i];
        memcpy(sortedUserNameList[i], userNameList[i], 32);
        //Serial.printf("Loaded user name: %s\n", sortedUserNameList[i]);
    }
/*
    for(int i = 0; i < ORDERED_LIST_LEN; i++)
    {
        //Serial.printf("Normal index %i: username is %s, rssi is %i.\n Sorted index: username is %s, rssi is %i\n\n", i, userNameList[i], rssi[i], sortedUserNameList[i], sortedRssi[i]);
    }
*/
}


int8_t DisplayInfo::getRssi(int i)
{
    //Serial.printf("Returning %i\n", i);
    return this->sortedRssi[i];
}


char *DisplayInfo::getUserName(int i)
{
    //Serial.printf("Sending username %s\n", sortedUserNameList[i]);
    return this->sortedUserNameList[i];
}

//Sort by closest peers based off of rssi
void DisplayInfo::sortPeers()
{
    int iterations = 0;
    int peers = getNumCurPeer();
    if(peers > 1)
    {
        //Simple bubble sort
        bool swap = false;
        for(int i = 0; i < peers; i++)
        {
            for(int j = 0; j < peers - i - 1; j++)
            {
            //iterations++;
                int8_t rssiPlaceHolder;
                char namePlaceHolder[32];
                Serial.print("Comparing ");
                Serial.print( sortedRssi[j]);
                Serial.print(" to ");
                Serial.println(sortedRssi[j+1]);

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
                        //Serial.printf("Error may be happening at %i\n", i);
                    }
                }

                
            }
            if(swap == false)
            {
                break;
            }
        }
    }
    for(int i = 0; i < 32; i++)
    {
        Serial.print("Rssi: ");
        Serial.print(this->sortedRssi[i]);
        Serial.print(" username: ");
        Serial.println(this->sortedUserNameList[i]);
    }
    Serial.printf("Iterations: %i\n", iterations);
    Serial.println();
    Serial.println();
    Serial.println();
}