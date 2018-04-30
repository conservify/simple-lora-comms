#include <lwstreams/lwstreams.h>
#include <utility>
#include <cstdio>

#include "protocol.h"
#include "debug.h"
#include "file_writer.h"
#include "timer.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <wiringPi.h>
#endif

#ifndef ARDUINO
inline int32_t random(int32_t min, int32_t max) {
    return (rand() % (max - min)) + min;
}
#endif

static Timer transmitting;
static Timer waitingOnAck;

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
    logger << "Radio: S " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " (" << stream.bytes_written << " bytes)\n";
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
    fklogln("Radio: %s -> %s", getStateName(state), getStateName(newState));
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

void NodeNetworkProtocol::tick() {
    if (getRadio()->isModeTx()) {
        if (!transmitting.isRunning()) {
            transmitting.begin();
        }
    }
    else if (transmitting.isRunning()) {
        transmitting.end();
    }
    switch (getState()) {
    case NetworkState::Starting: {
        retries().clear();
        transition(NetworkState::Listening);
        break;
    }
    case NetworkState::Idle: {
        getRadio()->setModeIdle();
        if (isTimerDone()) {
            transition(NetworkState::Listening);
        }
        break;
    }
    case NetworkState::Sleeping: {
        getRadio()->setModeIdle();
        if (isTimerDone()) {
            transition(NetworkState::Listening);
        }
        break;
    }
    case NetworkState::Listening: {
        retries().clear();
        getRadio()->setModeRx();
        if (inStateFor(ListeningWindowLength)) {
            transition(NetworkState::PingGateway);
        }
        break;
    }
    case NetworkState::PingGateway: {
        sendPacket(RadioPacket{ fk_radio_PacketKind_PING, deviceId });
        transition(NetworkState::WaitingForPong);
        break;
    }
    case NetworkState::WaitingForPong: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                fklogln("Radio: FAIL!");
                transition(NetworkState::Listening);
            }
        }
        break;
    }
    case NetworkState::Prepare: {
        if (reader != nullptr) {
            delete reader;
        }
        reader = new lws::CountingReader(4096);
        auto prepare = RadioPacket{ fk_radio_PacketKind_PREPARE, deviceId };
        prepare.m().size = 4096;
        sendPacket(std::move(prepare));
        transition(NetworkState::WaitingForReady);
        waitingOnAck.begin();
        break;
    }
    case NetworkState::WaitingForReady: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                if (retries().canRetry()) {
                    fklogln("Radio: RETRY!");
                    transition(NetworkState::Prepare);
                }
                else {
                    fklogln("Radio: FAIL!");
                    transition(NetworkState::Listening);
                }
            }
        }
        break;
    }
    case NetworkState::ReadData: {
        auto bp = buffer.toBufferPtr();
        auto bytes = reader->read(bp.ptr, bp.size);
        if (bytes < 0) {
            transition(NetworkState::Idle, random(IdleWindowMin, IdleWindowMax));
            logger << "Done! waitingOnAck: " << waitingOnAck << " transmitting: " << transmitting << "\n";
        }
        else if (bytes > 0) {
            buffer.position(bytes);
            transition(NetworkState::SendData);
        }
        break;
    }
    case NetworkState::SendData: {
        auto packet = RadioPacket{ fk_radio_PacketKind_DATA, deviceId };
        packet.data(buffer.toBufferPtr().ptr, buffer.position());
        sendPacket(std::move(packet));
        transition(NetworkState::WaitingForSendMore);
        waitingOnAck.begin();
        break;
    }
    case NetworkState::WaitingForSendMore: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                if (retries().canRetry()) {
                    fklogln("Radio: RETRY!");
                    transition(NetworkState::SendData);
                }
                else {
                    fklogln("Radio: FAIL!");
                    transition(NetworkState::Listening);
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    }
}

void NodeNetworkProtocol::push(LoraPacket &lora) {
    auto packet = RadioPacket{ lora };
    auto traffic = packet.m().kind != fk_radio_PacketKind_ACK && packet.getDeviceId() != deviceId;

    logger << "Radio: R " << lora.id << " " << packet.m().kind << " (" << lora.size << " bytes)" << (traffic ? " TRAFFIC" : "") << "\n";

    switch (getState()) {
    case NetworkState::Listening: {
        if (traffic) {
            transition(NetworkState::Sleeping, random(SleepingWindowMin, SleepingWindowMax));
            return;
        }
        break;
    }
    case NetworkState::WaitingForPong: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PONG: {
            retries().clear();
            logger << "Radio: Pong: My address: " << packet.m().address << "\n";
            transition(NetworkState::Prepare);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForReady: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_ACK: {
            waitingOnAck.end();
            zeroSequence();
            bumpSequence();
            retries().clear();
            transition(NetworkState::ReadData);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForSendMore: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_ACK: {
            waitingOnAck.end();
            bumpSequence();
            retries().clear();
            transition(NetworkState::ReadData);
            break;
        }
        }
        break;
    }
    default: {
        break;
    }
    }

    #ifdef ARDUINO
    randomSeed(millis());
    #endif
}

void GatewayNetworkProtocol::tick() {
    switch (getState()) {
    case NetworkState::Starting: {
        transition(NetworkState::Listening);
        break;
    }
    case NetworkState::Idle: {
        getRadio()->setModeRx();
        break;
    }
    case NetworkState::Listening: {
        getRadio()->setModeRx();
        break;
    }
    case NetworkState::SendPong: {
        break;
    }
    default: {
        break;
    }
    }
}

void GatewayNetworkProtocol::push(LoraPacket &lora) {
    auto packet = RadioPacket{ lora };

    logger << "Radio: R " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " (" << lora.size << " bytes)\n";

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PING: {
            delay(ReplyDelay);
            auto pong = RadioPacket{ fk_radio_PacketKind_PONG, packet.getDeviceId() };
            pong.m().address = nextAddress++;
            sendPacket(std::move(pong));
            break;
        }
        case fk_radio_PacketKind_PREPARE: {
            if (writer != nullptr) {
                writer->close();
                delete writer;
            }
            auto fileWriter = new FileWriter("DATA");
            if (!fileWriter->open()) {
                delete fileWriter;
                fileWriter = nullptr;
            }
            writer = fileWriter;
            totalReceived = 0;
            receiveSequence = 0;
            delay(ReplyDelay);
            sendAck(lora.from);
            break;
        }
        case fk_radio_PacketKind_DATA: {
            auto data = packet.data();
            auto dupe = lora.id <= receiveSequence;
            if (!dupe) {
                if (writer != nullptr) {
                    auto written = writer->write(data.ptr, data.size);
                    assert(written == data.size);
                }
                totalReceived += data.size;
                receiveSequence = lora.id;
            }
            logger << "Radio: R " << totalReceived << " " << lora.id << " " << receiveSequence << " " << (dupe ? "DUPE" : "") << "\n";
            delay(ReplyDelay);
            sendAck(lora.from);
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    }
}
