#ifndef PACKETINFO_H
#define PACKETINFO_H

#include <stdint.h>
#include <selfInfo.h>


class PacketInfo : SelfInfo
{
    char *userName;
    uint8_t *selfIdent;

    public:
    void setUsername;
    void setIdent
};


#endif