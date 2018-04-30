#ifndef FK_PACKET_RADIO_H_INCLUDED
#define FK_PACKET_RADIO_H_INCLUDED

#include "packets.h"

constexpr float LoraRadioFrequency = 915.0;
constexpr uint8_t LoraRadioMaximumRetries = 3;

class PacketRadio {
public:
    virtual bool isModeRx() = 0;
    virtual bool isModeTx() = 0;
    virtual bool isIdle() = 0;
    virtual void setModeRx() = 0;
    virtual void setModeIdle() = 0;
    virtual bool sendPacket(LoraPacket &packet) = 0 ;
    virtual void setThisAddress(uint8_t address) = 0;

};

#endif
