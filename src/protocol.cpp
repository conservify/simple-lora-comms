#include <lwstreams/lwstreams.h>
#include <utility>
#include <cstdio>

#include "protocol.h"

Timer transmitting;
Timer waitingOnAck;

bool NetworkProtocol::sendPacket(RadioPacket &&packet) {
    size_t required = 0;
    if (!pb_get_encoded_size(&required, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    char buffer[required];
    auto stream = pb_ostream_from_buffer((uint8_t *)buffer, required);
    if (!pb_encode(&stream, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    LoraPacket lora;
    lora.id = sequence;
    memcpy(lora.data, buffer, stream.bytes_written);
    lora.size = stream.bytes_written;
    slc::log() << "S " << packet.m().kind << " " << packet.getNodeId() << " " << lora.id << " (" << stream.bytes_written << " bytes)";
    return radio->sendPacket(lora);
}

bool NetworkProtocol::sendAck(uint8_t toAddress) {
    LoraPacket ack;
    ack.id = sequence;
    ack.to = toAddress;
    ack.flags = 1;
    ack.size = 0;
    return radio->sendPacket(ack);
}

void NetworkProtocol::transition(NetworkState newState, uint32_t timer) {
    lastTransitionAt = millis();
    slc::log() << getStateName(state) << " -> " << getStateName(newState);
    state = newState;
    if (timer > 0) {
        timerDoneAt = millis() + timer;
    }
    else {
        timerDoneAt = 0;
    }
}

bool NetworkProtocol::isTimerDone() {
    return timerDoneAt > 0 && millis() > timerDoneAt;
}

bool NetworkProtocol::inStateFor(uint32_t ms) {
    return millis() - lastTransitionAt > ms;
}
