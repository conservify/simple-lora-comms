#include <cassert>

#include <SPI.h>
#include <RH_RF95.h>

#include "protocol.h"

constexpr size_t FK_QUEUE_ENTRY_SIZE = 242;

class LoraRadio : public PacketRadio {
private:
    RH_RF95 rf95;
    uint8_t pinCs;
    uint8_t pinReset;
    uint8_t pinEnable;
    bool available{ false };

public:
    LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinReset);
    bool setup();
    void tick();
    bool sendPacket(LoraPacket &packet) override;

    void waitPacketSent() {
        rf95.waitPacketSent();
    }

    bool hasPacket() {
        return rf95.available();
    }

    LoraPacket getLoraPacket();

private:
    void powerOn() {
        if (pinEnable > 0) {
            digitalWrite(pinEnable, HIGH);
        }
    }

    void powerOff() {
        if (pinEnable > 0) {
            digitalWrite(pinEnable, LOW);
        }
    }

    void reset() {
        if (pinEnable > 0) {
            powerOff();
            delay(10);
            powerOn();
            delay(10);
        }
    }

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
        return getMode() == RHGenericDriver::RHMode::RHModeIdle;
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
