#ifndef SELFINFO_H
#define SELFINFO_H

#include <stdint.h>

class SelfInfo 
{
    uint8_t selfIdentifier[16];
    char *userName;
    
    public:
    SelfInfo();
    uint8_t *getSelfIdentifier();
    void setUserName(char* userName);
};

#endif