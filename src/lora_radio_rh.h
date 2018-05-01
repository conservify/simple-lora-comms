#ifndef SLC_LORA_RADIO_RH_INCLUDED
#define SLC_LORA_RADIO_RH_INCLUDED
#ifdef ARDUINO

#include <cassert>
#include <SPI.h>
#include <RH_RF95.h>

#include "protocol.h"

constexpr size_t FK_QUEUE_ENTRY_SIZE = 242;

class LoraRadioRadioHead : public PacketRadio {
private:
    RH_RF95 rf95;
    uint8_t pinCs;
    uint8_t pinReset;
    uint8_t pinEnable;
    bool available{ false };

public:
    LoraRadioRadioHead(uint8_t pinCs, uint8_t pinD0, uint8_t pinEnable, uint8_t pinReset);

public:
    bool setup();
    bool sendPacket(LoraPacket &packet) override;
    bool hasPacket();
    LoraPacket getLoraPacket();

private:
    void powerOn();
    void powerOff();
    void reset();

public:
    uint8_t getMode() {
        return (uint8_t)rf95.mode();
    }

    bool isModeRx() override {
        return getMode() == RHGenericDriver::RHModeRx;
    }

    bool isModeTx() override {
        return getMode() == RHGenericDriver::RHModeTx;
    }

    bool isIdle() override {
        return getMode() == RHGenericDriver::RHModeIdle;
    }

    void sleep() override {
        rf95.sleep();
    }

    void setModeIdle() override {
        rf95.setModeIdle();
    }

    void setModeRx() override {
        rf95.setModeRx();
    }

    void setThisAddress(uint8_t value) {
        rf95.setThisAddress(value);
    }

    void setHeaderFrom(uint8_t value) {
        rf95.setHeaderFrom(value);
    }

    uint8_t headerFrom() {
        return rf95.headerFrom();
    }

    void setHeaderTo(uint8_t value) {
        rf95.setHeaderTo(value);
    }

    uint8_t headerTo() {
        return rf95.headerTo();
    }

    void setHeaderFlags(uint8_t value) {
        rf95.setHeaderFlags(value);
    }

    uint8_t headerFlags() {
        return rf95.headerFlags();
    }

    void setHeaderId(uint8_t value) {
        rf95.setHeaderId(value);
    }

    uint8_t headerId() {
        return rf95.headerId();
    }

};

#endif
#endif
