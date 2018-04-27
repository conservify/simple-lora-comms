#include "lora_radio.h"

constexpr float LoraRadioFrequency = 915.0;
constexpr uint8_t LoraRadioMaximumRetries = 3;

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinRst)
    : pinCs(pinCs), pinRst(pinRst), rf95(pinCs, pinG0), pinEnable(pinEnable), available(false) {
}

bool LoraRadio::setup() {
    if (available) {
        return true;
    }

    pinMode(pinCs, OUTPUT);
    digitalWrite(pinCs, HIGH);

    pinMode(pinRst, OUTPUT);
    digitalWrite(pinRst, HIGH);

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

    available = true;
    return true;
}

bool LoraRadio::send(uint8_t *packet, uint8_t size) {
    memcpy(sendBuffer, packet, size);
    sendLength = size;
    return rf95.send(sendBuffer, sendLength);
}

bool LoraRadio::sendPacket(RadioPacket &packet) {
    size_t required = 0;
    if (!pb_get_encoded_size(&required, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    char buffer[required];
    auto stream = pb_ostream_from_buffer((uint8_t *)buffer, required);
    if (!pb_encode(&stream, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    auto id = sequence++;
    rf95.setHeaderId(id);

    logger << "Radio: S " << id << " " << packet.m().kind << " " << packet.getDeviceId() << " (" << stream.bytes_written << " bytes)\n";

    return send((uint8_t *)buffer, stream.bytes_written);
}

void LoraRadio::tick() {
    if (isModeRx()) {
        if (rf95.available()) {
            recvLength = sizeof(recvBuffer);
            rf95.recv(recvBuffer, &recvLength);
        }
        else {
            recvLength = 0;
        }
    }
}
