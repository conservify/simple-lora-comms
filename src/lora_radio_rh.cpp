#ifdef ARDUINO

#include "lora_radio_rh.h"

LoraRadioRadioHead::LoraRadioRadioHead(uint8_t pinCs, uint8_t pinD0, uint8_t pinEnable, uint8_t pinReset)
    : pinCs(pinCs), pinReset(pinReset), rf95(pinCs, pinD0), pinEnable(pinEnable), available(false) {
}

bool LoraRadioRadioHead::setup() {
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
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
    rf95.spiWrite(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xF2);

    available = true;
    return true;
}

bool LoraRadioRadioHead::sendPacket(LoraPacket &packet) {
    rf95.setHeaderTo(packet.to);
    rf95.setHeaderFrom(packet.from);
    rf95.setHeaderId(packet.id);
    rf95.setHeaderFlags(packet.flags);
    return rf95.send(packet.data, packet.size);
}

LoraPacket LoraRadioRadioHead::getLoraPacket() {
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

#endif
