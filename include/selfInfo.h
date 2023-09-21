#ifndef SELFINFO_H
#define SELFINFO_H

#include <stdint.h>

class SelfInfo 
{
    uint8_t selfIdentifier[16];
    
    public:
    SelfInfo();
    uint8_t *getSelfIdentifier();
};

#endif