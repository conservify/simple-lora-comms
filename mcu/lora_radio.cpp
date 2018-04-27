#include "lora_radio.h"

constexpr float LoraRadioFrequency = 915.0;
constexpr uint8_t LoraRadioMaximumRetries = 3;

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinReset)
    : pinCs(pinCs), pinReset(pinReset), rf95(pinCs, pinG0), pinEnable(pinEnable), available(false) {
}

bool LoraRadio::setup() {
    if (available) {
        return true;
    }

    pinMode(pinCs, OUTPUT);
    digitalWrite(pinCs, HIGH);

    pinMode(pinReset, OUTPUT);
    digitalWrite(pinReset, HIGH);

    pinMode(pinEnable, OUTPUT);
    digitalWrite(pinEnable, HIGH);

    powerOn();
    delay(10);
    reset();

    if (!rf95.init()) {
        return false;
    }

    if (!rf95.setFrequency(LoraRadioFrequency)) {
        return false;
    }

    rf95.setTxPower(23, false);

    rf95.spiWrite(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xF2);

    available = true;
    return true;
}

bool LoraRadio::sendPacket(LoraPacket &packet) {
    rf95.setHeaderTo(packet.to);
    rf95.setHeaderFrom(packet.from);
    rf95.setHeaderId(packet.id);
    rf95.setHeaderFlags(packet.flags);
    return rf95.send(packet.data, packet.size);
}

void LoraRadio::tick() {

}

LoraPacket LoraRadio::getLoraPacket() {
    LoraPacket packet;
    packet.to = rf95.headerTo();
    packet.from = rf95.headerFrom();
    packet.id = rf95.headerId();
    packet.flags = rf95.headerFlags();
    packet.size = sizeof(packet.data);
    auto size = (uint8_t)sizeof(packet.data);
    rf95.recv(packet.data, &size);
    packet.size = size;
    return packet;
}
