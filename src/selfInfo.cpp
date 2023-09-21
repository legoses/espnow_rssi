#include <selfInfo.h>

#include <bootloader_random.h>

SelfInfo::SelfInfo()
{
    bootloader_random_enable();
    //selfIdentifier = esp_random();
    bootloader_fill_random(selfIdentifier, 16);
    bootloader_random_disable();
}


uint8_t *SelfInfo::getSelfIdentifier()
{
    return this->selfIdentifier;
}