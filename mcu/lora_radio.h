#include <SPI.h>
#include <RH_RF95.h>

constexpr size_t FK_QUEUE_ENTRY_SIZE = 242;

class LoraRadio {
private:
    RH_RF95 rf95;
    uint8_t pinCs;
    uint8_t pinRst;
    uint8_t pinEnable;
    bool available;
    uint8_t sendBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t sendLength;
    uint8_t recvBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t recvLength;
    uint8_t tries;

public:
    LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinRst);
    bool setup();
    void tick();
    bool send(uint8_t *packet, uint8_t size);
    bool reply(uint8_t *packet, uint8_t size);
    bool resend();

    uint8_t numberOfTries() {
        return tries;
    }

    bool isAvailable() {
        return available;
    }

    const uint8_t *getPacket() {
        return recvBuffer;
    }

    uint8_t getPacketSize() {
        return recvLength;
    }

    bool hasPacket() {
        return recvLength > 0;
    }

    bool isIdle() {
        return rf95.mode() == RHGenericDriver::RHMode::RHModeIdle;
    }

    void clear() {
        recvBuffer[0] = 0;
        recvLength = 0;
    }

    void wake() {
    }

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

    void idle() {
        rf95.setModeIdle();
    }

    void reset() {
        powerOff();
        delay(10);
        powerOn();
        delay(10);
    }

    void waitPacketSent() {
        rf95.waitPacketSent();
    }

    void sleep() {
        rf95.sleep();
    }

    void printRegisters() {
        rf95.printRegisters();
    }

    RHGenericDriver::RHMode mode() {
        return rf95.mode();
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

    uint8_t lastRssi() {
        return rf95.lastRssi();
    }

};
